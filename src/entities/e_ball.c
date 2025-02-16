#include "assets.h"
#include "entities.h"
#include "types.h"
#include "math_ext.h"
#include "log.h"
#include "macro.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <string.h>
#include "game.h"

implement_state_machine_functions(ball);

static void ball_prepare_gfx(struct component_ball *ball, struct game_state *game_state);

struct entity *entities_ball_create(struct ball_config config, struct game_state *game_state)
{
	struct entity *entity =
		entities_entity_init(game_state->entity_repository, ET_BALL, "ball_%02d");

	// Initialise transform
	struct component_transform *transform = &entity->transform;
	transform->header.enabled = true;
	transform->position = config.position;
	transform->rotation = 0;
	transform->virtual_height = 100;
	transform->scale = (v2){.x = 0.625, .y = 0.625};

	// Initialise 3d physics
	struct component_physics3d *physics3d = &entity->physics3d;
	physics3d->header.enabled = true;
	physics3d->mass = 0.43f;
	physics3d->velocity = (Vector3){0};
	physics3d->accel = (Vector3){0};
	physics3d->max_velocity = game_state->constants.ball_max_velocity;

	// Initialise sprite
	struct component_sprite *sprite = &entity->sprite;
	char *ball_frame_name = "ball_left_0";
	struct assets_sprite_frame *sprite_frame =
		assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, ball_frame_name);
	assert(sprite_frame != NULL);
	sprite->header.enabled = true;
	sprite->frame_id = sprite_frame->id;
	sprite->color = WHITE;
	sprite->flipX = false;
	sprite->flipY = false;
	sprite->pivot_override = (v2){.x = 0.5, .y = 0.5};
	sprite->layer = SL_PLAYERS_AND_BALL;
	sprite->draw_shadow = true;
	sprite->shadow_color = BLACK;
	sprite->shadow_color.a = 100;
	sprite->shadow_offset = (v2){.x = 0.02, .y = -0.01};
	sprite->shadow_scale = (v2){.x = 1, .y = 1};

	// Initialise collider
	struct component_collider *collider = &entity->collider;
	collider->header.enabled = true;
	collider->colliders =
		memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(struct collider));
	collider->num_colliders = 1;
	collider->colliders[0].shape = CS_CIRCLE;
	collider->colliders[0].props.circle.radius = 2.5f;

	// Initialise action
	entity->action.header.enabled = true;

	// Initialise ball
	struct component_ball *ball = &entity->ball;
	ball->header.enabled = true;
	struct state_machine_ball *state_machine =
		memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(*state_machine));
	state_machine->owner_entity = entity->header.id;
	state_machine->previous_state = BS_NONE;
	state_machine->current_state = BS_IN_PLAY;
	ball->state_machine = state_machine;
	ball->last_touch_entity_id = EID_NOT_VALID;
	ball->last_kicker_entity_id = EID_NOT_VALID;
	ball->pass_target_entity_id = EID_NOT_VALID;
	ball->last_touch_time = EID_NOT_VALID;

	ball_prepare_gfx(ball, game_state);

	entities_ball_state_machine_on_enter(entity, game_state);

	GLOG_INFO_CTX(LS_ENTITIES, entity->header.name, "ball created");

	return entity;
}

void entities_ball_state_machine_on_enter(struct entity *entity, struct game_state *game_state)
{
	UNUSED(game_state);

	struct component_ball *ball = &entity->ball;
	struct state_machine_ball *state_machine = ball->state_machine;

	GLOGF_INFO_CTX(LS_ENTITIES,
				   entity->header.name,
				   "entering state: %s",
				   state_machine_ball_state_string[state_machine->current_state]);

	// Reset state data
	memset(&state_machine->state_data, 0, sizeof(union state_machine_ball_state_data));

	switch (state_machine->current_state) {
	case BS_NONE:
		break;
	case BS_IN_PLAY: {
		break;
	}
	case BS_OUT_OF_PLAY: {
		break;
	}
	case BS_GK_HANDS: {
		struct component_sprite *sprite = &entity->sprite;
		sprite->header.enabled = false;
	}
	case BS_COUNT:
		break;
	}
}

void entities_ball_state_machine_on_exit(struct entity *entity, struct game_state *game_state)
{
	UNUSED(game_state);

	struct component_sprite *sprite = &entity->sprite;
	struct component_ball *ball = &entity->ball;
	struct state_machine_ball *state_machine = ball->state_machine;

	GLOGF_INFO_CTX(LS_ENTITIES,
				   entity->header.name,
				   "exiting state: %s",
				   state_machine_ball_state_string[state_machine->previous_state]);

	switch (state_machine->previous_state) {
	case BS_NONE:
		break;
	case BS_IN_PLAY: {
		break;
	}
	case BS_OUT_OF_PLAY: {
		break;
	}
	case BS_GK_HANDS: {
		sprite->header.enabled = true;
		break;
	}
	case BS_COUNT:
		break;
	}
}

static void ball_prepare_gfx(struct component_ball *ball, struct game_state *game_state)
{
	assets_animation_cache_t *animation_cache = game_state->assets.animation_cache;
	memory_arena_handle_t *memory_arena_handle = game_state->memory_by_lifetime[MAL_UPDATE];

	// Roll frames
	ball->roll_animations[SO_UP] =
		assets_animation_id_from_name(animation_cache, "ball", "up", memory_arena_handle);
	ball->roll_animations[SO_DOWN] =
		assets_animation_id_from_name(animation_cache, "ball", "down", memory_arena_handle);
	ball->roll_animations[SO_LEFT] =
		assets_animation_id_from_name(animation_cache, "ball", "left", memory_arena_handle);
	ball->roll_animations[SO_RIGHT] =
		assets_animation_id_from_name(animation_cache, "ball", "right", memory_arena_handle);
	ball->roll_animations[SO_UP_LEFT] =
		assets_animation_id_from_name(animation_cache, "ball", "up_left", memory_arena_handle);
	ball->roll_animations[SO_UP_RIGHT] =
		assets_animation_id_from_name(animation_cache, "ball", "up_right", memory_arena_handle);
	ball->roll_animations[SO_DOWN_LEFT] =
		assets_animation_id_from_name(animation_cache, "ball", "down_left", memory_arena_handle);
	ball->roll_animations[SO_DOWN_RIGHT] =
		assets_animation_id_from_name(animation_cache, "ball", "down_right", memory_arena_handle);
}

void entities_ball_set_last_touch(struct component_ball *ball,
								  entity_id_t player_entity_id,
								  struct game_state *game_state)
{
	UNUSED(game_state);

	if (player_entity_id == EID_NOT_VALID) {
		ball->last_touch_time = 0;
		ball->first_player_possession_time = 0;
	} else {
		if (ball->last_touch_entity_id != player_entity_id ||
			HAS_TIME_PASSED(game_state->time, ball->last_touch_time, 0.5f)) {
			// Possession changed or a lot of time passed from last possession of the same player, so it counts as a new one
			ball->first_player_possession_time = game_state->time;
		}
		ball->last_touch_time = game_state->time;
	}

	ball->last_touch_entity_id = player_entity_id;
}

void entities_ball_set_position_and_velocity(struct entity *ball_entity, v3 pos, v3 velocity)
{
	struct component_transform *transform = &ball_entity->transform;
	transform->position = V3_TO_V2(pos);
	transform->virtual_height = pos.y;
	ball_entity->physics3d.accel = Vector3Zero();
	ball_entity->physics3d.velocity = velocity;
	ball_entity->ball.future_bounce.available = false;
	ball_entity->ball.future_reachable.available = false;
	ball_entity->ball.subject_to_after_touch = false;
	ball_entity->ball.selected_after_touch_side = Vector2Zero();
}

void entities_ball_control_precision(struct entity *ball_entity,
									 v2 player_velocity,
									 v2 position_to_seek,
									 f32 difficulty,
									 struct game_state *game_state)
{
	struct component_transform *transform = &ball_entity->transform;
	struct component_physics3d *physics3d = &ball_entity->physics3d;

	// The more the difficulty of the control, the less force will be applied to the ball.
	// This means it will be easier for another player to steal the ball.
	physics3d->velocity = Vector3Add(
		V2_TO_V3(player_velocity, 0),
		Vector3Scale(
			Vector3Normalize(
				Vector3Subtract(V2_TO_V3(position_to_seek, 0),
								V2_TO_V3(transform->position, transform->virtual_height))),
			math_ext_split_lerp(game_state->constants.ball_control_force, 0.5f, 1 - difficulty)));
}

void entities_ball_control_running(struct entity *ball_entity,
								   v2 control_direction,
								   v2 player_velocity,
								   f32 difficulty,
								   struct game_state *game_state)
{
	struct component_transform *transform = &ball_entity->transform;
	struct component_physics3d *physics3d = &ball_entity->physics3d;

	f32 vertical_force = transform->virtual_height > 10 ? 0 : 0.5f;
	v3 velocity_to_add = Vector3Scale(
		Vector3Add(Vector3Normalize(V2_TO_V3(control_direction, 0)), (v3){.y = vertical_force}),
		math_ext_split_lerp(game_state->constants.ball_run_force, 0.25f, difficulty));
	physics3d->velocity = Vector3Add(V2_TO_V3(player_velocity, 0), velocity_to_add);
}

void entities_ball_shoot(struct entity *ball_entity,
						 entity_id_t kicker_entity_id,
						 v2 direction,
						 f32 height,
						 f32 power,
						 struct game_state *game_state)
{
	// struct component_transform *transform = &entity->transform;
	struct component_physics3d *physics3d = &ball_entity->physics3d;
	struct component_ball *ball = &ball_entity->ball;
	// struct component_collider *collider = &entity->collider;

	v3 shot_direction = Vector3Add(
		V2_TO_V3(direction, 0),
		(Vector3){.x = 0,
				  .y = math_ext_split_lerp(game_state->constants.ball_shot_height, 0.2f, height),
				  .z = 0});
	if (game_state->constants.shoot_aid_no_inertia) {
		// reset velocity to avoid existing inertia affecting the trajectory: not sure this is
		// better though, need to test
		physics3d->velocity = Vector3Zero();
	}
	// Calculate shot force
	// Max power is achieved at 80%, if you go over you actually lose power
	if (power > 0.8f) {
		power = 0.8f - 0.2f * ((power - 0.8f) / 0.2f);
	} else {
		power = power / 0.8f;
	}
	f32 shot_force = math_ext_split_lerp(game_state->constants.ball_shot_force, 0.6f, power);
	// f32 ball_radius = collider->colliders[0].props.circle.radius;
	// if (!ball->is_on_the_ground) {
	//     if (transform->virtual_height > (ball_radius + 12)) {
	//         // TODO: this needs to handle headers
	//         GLOG_DEBUG_CTX(LS_GAME, entity->header.name, "ball shot, should be an header");
	//     } else if (transform->virtual_height > ball_radius) {
	//         if (physics3d->velocity.y < 0) {
	//             // Ball is falling
	//             shot_force *= 1.05f;
	//             GLOG_DEBUG_CTX(LS_GAME, entity->header.name, "ball shot during fall trajectory");
	//         } else if (physics3d->velocity.y > 0) {
	//             // If it's not falling then maybe it's a drop shot?
	//             if (IS_TIME_PASSED_IN_RANGE(game_state->time, ball->last_bounce_time, 0, 0.25f))
	//             {
	//                 // Drop shot
	//                 shot_force *= 1.10;
	//                 GLOG_DEBUG_CTX(LS_GAME, entity->header.name, "ball drop shot");
	//             } else if (IS_TIME_PASSED_IN_RANGE(game_state->time, ball->last_bounce_time,
	//             0.25f,
	//                                                0.5f)) {
	//                 // Nope, this must be the shittiest shot ever
	//                 shot_force *= 0.8f;
	//                 GLOG_DEBUG_CTX(LS_GAME, entity->header.name, "ball drop shot too late");
	//             }
	//         }
	//     }
	// }

	physics3d->accel =
		Vector3Add(physics3d->accel, Vector3Scale(shot_direction, shot_force / game_state->dt));
	ball->last_kick_time = game_state->time;
	ball->last_kicker_entity_id = kicker_entity_id;
	ball->subject_to_after_touch = true;
	ball->selected_after_touch_side = Vector2Zero();
}

void entities_ball_short_pass(struct entity *ball_entity,
							  entity_id_t kicker_entity_id,
							  v3 direction,
							  f32 power,
							  b32 auto_pass,
							  struct game_state *game_state)
{
	struct component_physics3d *physics3d = &ball_entity->physics3d;
	struct component_ball *ball = &ball_entity->ball;

	if (auto_pass) {
		// reset velocity to avoid existing inertia affecting the trajectory
		physics3d->velocity = Vector3Zero();
	}
	physics3d->accel = Vector3Add(
		physics3d->accel,
		Vector3Scale(direction,
					 math_ext_split_lerp(game_state->constants.ball_pass_force, 0.5f, power) /
						 game_state->dt));
	ball->last_kick_time = game_state->time;
	ball->last_kicker_entity_id = kicker_entity_id;
}

void entities_ball_long_pass(struct entity *ball_entity,
							 entity_id_t kicker_entity_id,
							 v2 direction,
							 f32 power,
							 struct game_state *game_state)
{
	struct component_physics3d *physics3d = &ball_entity->physics3d;
	struct component_ball *ball = &ball_entity->ball;

	v3 long_pass_direction =
		Vector3Add(Vector3Normalize(V2_TO_V3(direction, 0)),
				   (v3){.x = 0, .y = game_state->constants.ball_long_pass_height * power, .z = 0});
	physics3d->accel = Vector3Add(
		physics3d->accel,
		Vector3Scale(long_pass_direction,
					 math_ext_split_lerp(game_state->constants.ball_long_pass_force, 0.75f, power) /
						 game_state->dt));
	ball->last_kick_time = game_state->time;
	ball->last_kicker_entity_id = kicker_entity_id;
	ball->subject_to_after_touch = true;
	ball->selected_after_touch_side = Vector2Zero();
}

void entities_ball_tackle(struct entity *ball_entity,
						  entity_id_t kicker_entity_id,
						  v3 direction,
						  f32 power,
						  struct game_state *game_state)
{
	struct component_physics3d *physics3d = &ball_entity->physics3d;
	struct component_ball *ball = &ball_entity->ball;

	physics3d->velocity = Vector3Zero();
	physics3d->accel = Vector3Add(
		physics3d->accel,
		Vector3Scale(direction,
					 math_ext_split_lerp(game_state->constants.ball_tackle_force, 0.5f, power) /
						 game_state->dt));
	ball->last_kick_time = game_state->time;
	ball->last_kicker_entity_id = kicker_entity_id;
}

void entities_ball_deviate_trajectory(struct entity *ball_entity,
									  entity_id_t player_entity_id,
									  struct game_state *game_state)
{
	struct component_physics3d *physics3d = &ball_entity->physics3d;
	struct component_ball *ball = &ball_entity->ball;

	v3 random_deviation = {
		.x = GetRandomValue(-100, 100),
		.y = GetRandomValue(-100, 100),
		.z = GetRandomValue(-100, 100),
	};
	random_deviation = Vector3Normalize(random_deviation);
	physics3d->velocity = Vector3Zero();
	physics3d->accel =
		Vector3Add(physics3d->accel,
				   Vector3Scale(random_deviation,
								math_ext_split_lerp(game_state->constants.ball_tackle_force,
													0.0f,
													GetRandomValue(25, 100) / 100.f) /
									game_state->dt));
	ball->last_kick_time = game_state->time;
	ball->last_kicker_entity_id = player_entity_id;
}

void entities_ball_held_by_goalie(struct entity *ball_entity,
								  entity_id_t goalie_entity_id,
								  struct game_state *game_state)
{
	struct component_ball *ball = &ball_entity->ball;

	entities_ball_set_last_touch(ball, goalie_entity_id, game_state);

	entities_ball_state_machine_change_state_to(ball->state_machine, BS_GK_HANDS, game_state);
}

void entities_ball_saved_by_goalie(struct entity *ball_entity,
								   entity_id_t goalie_entity_id,
								   struct game_state *game_state)
{
	struct component_ball *ball = &ball_entity->ball;

	entities_ball_set_last_touch(ball, goalie_entity_id, game_state);
}

void entities_ball_back_in_play(struct entity *ball_entity, struct game_state *game_state)
{
	struct component_ball *ball = &ball_entity->ball;

	entities_ball_state_machine_change_state_to(ball->state_machine, BS_IN_PLAY, game_state);
}

void entities_ball_in_goal(struct entity *ball_entity)
{
	struct component_ball *ball = &ball_entity->ball;

	ball->subject_to_after_touch = false;
	ball->selected_after_touch_side = Vector2Zero();
}

entity_id_t entities_ball_player_in_possession(struct entity *ball_entity,
											   struct game_state *game_state)
{
	if (ball_entity->ball.last_touch_entity_id != EID_NOT_VALID &&
		HAS_TIME_PASSED(game_state->time, ball_entity->ball.first_player_possession_time, 0.1f) &&
		!HAS_TIME_PASSED(game_state->time, ball_entity->ball.last_touch_time, 1.0f) &&
		ball_entity->ball.last_touch_time > ball_entity->ball.last_kick_time) {
		return ball_entity->ball.last_touch_entity_id;
	} else {
		return EID_NOT_VALID;
	}
}

entity_id_t entities_ball_player_on_the_ball(struct entity *ball_entity,
											 struct game_state *game_state)
{
	if (ball_entity->ball.last_touch_entity_id != EID_NOT_VALID &&
		ball_entity->ball.last_touch_time == game_state->time) {
		return ball_entity->ball.last_touch_entity_id;
	} else {
		return EID_NOT_VALID;
	}
}
