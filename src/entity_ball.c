#include "entity_components.h"
#include "entity.h"
#include "entity_ball.h"
#include "events_router.h"
#include "state_machine.h"
#include "types.h"
#include "math_ext.h"
#include "log.h"
#include "macro.h"
#include "physics.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include <string.h>
#include "game.h"
#include "gfx.h"

#define BALL_IN_NET_DAMPING		   0.8f
#define BALL_IN_NET_RISING_DAMPING 0.9f

const char *state_machine_ball_state_string[BS_COUNT] = {
	"none", "in_play", "out_of_play", "gk_hands"};

STATE_MACHINE_FORWARD_DECLARE_FUNCTIONS(ball, struct entity);
STATE_MACHINE_IMPLEMENT_FUNCTIONS(ball, struct entity);

static void set_after_touch(struct component_ball *ball);
static b32 is_applied_after_touch_same_side(v2 stored_side, v2 selected_side);
static void reset_after_touch(struct component_ball *ball);

struct entity *entity_ball_create(struct ball_config config, struct game_state *game_state)
{
	struct entity *entity = entity_init(game_state->entity_repository, ET_BALL, "ball_%02d");

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
	struct asset_sprite_frame *sprite_frame =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, ball_frame_name);
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
	assert(collider->colliders);
	collider->num_colliders = 1;
	collider->colliders[0].shape = CS_CIRCLE;
	collider->colliders[0].props.circle.radius = 2.5f;

	// Initialise action
	entity->action.header.enabled = true;

	// Initialise ball
	struct component_ball *ball = &entity->ball;
	ball->header.enabled = true;
	struct state_machine_ball *state_machine = &entity->ball.state_machine;
	state_machine->ctx = entity;
	state_machine->previous_state = BS_NONE;
	state_machine->current_state = BS_IN_PLAY;
	ball->last_touch_entity_id = EID_NOT_VALID;
	ball->last_kicker_entity_id = EID_NOT_VALID;
	ball->pass_target_entity_id = EID_NOT_VALID;
	ball->last_touch_time = 0;

	gfx_ball_prepare(game_state);

	state_machine_ball_on_enter(entity, game_state);

	GLOG_INFO_CTX(LS_ENTITIES, entity->header.name, "ball created");

	return entity;
}

static void state_machine_ball_on_enter(struct entity *entity, struct game_state *game_state)
{
	UNUSED(game_state);

	struct component_ball *ball = &entity->ball;
	struct state_machine_ball *state_machine = &ball->state_machine;

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

static void state_machine_ball_on_exit(struct entity *entity, struct game_state *game_state)
{
	UNUSED(game_state);

	struct component_sprite *sprite = &entity->sprite;
	struct component_ball *ball = &entity->ball;
	struct state_machine_ball *state_machine = &ball->state_machine;

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

void entity_ball_set_last_touch(struct component_ball *ball,
								entity_id_t player_entity_id,
								struct game_state *game_state)
{
	UNUSED(game_state);

	if (player_entity_id == EID_NOT_VALID) {
		ball->last_touch_time = 0;
		ball->first_player_possession_time = 0;
	} else {
		if (ball->last_touch_entity_id != player_entity_id ||
			HAS_TIME_PASSED(game_state->time, ball->last_touch_time, 0.7f)) {
			// Possession changed or a lot of time passed from last possession of the same player, so it counts as a new one
			ball->first_player_possession_time = game_state->time;
		}
		ball->last_touch_time = game_state->time;
	}

	ball->last_touch_entity_id = player_entity_id;
	ball->pass_target_entity_id = EID_NOT_VALID;
}

void entity_ball_set_position_and_velocity(struct entity *ball_entity, v3 pos, v3 velocity)
{
	struct component_transform *transform = &ball_entity->transform;
	transform->position = V3_TO_V2(pos);
	transform->virtual_height = pos.y;
	ball_entity->physics3d.accel = Vector3Zero();
	ball_entity->physics3d.velocity = velocity;

	for (usize i = 0; i < FUTURE_POSITION_COUNT; i++) {
		ball_entity->ball.future_positions[i].available = false;
	}

	reset_after_touch(&ball_entity->ball);
}

void entity_ball_settle_in_net(struct entity *ball_entity,
							   rect_t goal_rectangle,
							   enum pitch_side side)
{
	struct component_transform *transform = &ball_entity->transform;
	struct component_physics3d *physics_3d = &ball_entity->physics3d;

	// Only damp while the ball is still short of the net or outside the posts: once it is in
	// the netting the normal physics settles it.
	b32 short_of_the_net =
		side == PSI_NORTH ? transform->position.y < goal_rectangle.y - 3 :
							transform->position.y >
								(goal_rectangle.y + goal_rectangle.height * 2 + 3);
	if (!short_of_the_net && transform->position.x >= goal_rectangle.x &&
		transform->position.x <= (goal_rectangle.x + goal_rectangle.width)) {
		return;
	}

	physics_3d->velocity.x *= BALL_IN_NET_DAMPING;
	physics_3d->velocity.z *= BALL_IN_NET_DAMPING;
	if (transform->virtual_height > goal_rectangle.height && physics_3d->velocity.y > 0) {
		physics_3d->velocity.y *= BALL_IN_NET_RISING_DAMPING;
	}
}

void entity_ball_control_precision(struct entity *ball_entity,
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
		V2_TO_V3(player_velocity, physics3d->velocity.y), // To keep ball existing vertical velocity
		Vector3Scale(
			Vector3Normalize(
				Vector3Subtract(V2_TO_V3(position_to_seek, 0), V2_TO_V3(transform->position, 0))),
			math_ext_split_lerp(game_state->constants.ball_control_force, 0.5f, 1 - difficulty)));

	reset_after_touch(&ball_entity->ball);
}

void entity_ball_control_running(struct entity *ball_entity,
								 v2 control_direction,
								 v2 player_velocity,
								 f32 difficulty,
								 struct game_state *game_state)
{
	struct component_physics3d *physics3d = &ball_entity->physics3d;

	// TODO: vertical component of the velocity to add should depend on the player's technique:
	// good players might have low value which translates to better control in all conditions,
	v3 velocity_to_add =
		Vector3Scale(V2_TO_V3(Vector2Normalize(control_direction),
							  ball_entity->ball.is_on_the_ground ? 0.5f : 0),
					 math_ext_split_lerp(game_state->constants.ball_run_force, 0.25f, difficulty));
	physics3d->velocity =
		Vector3Add(V2_TO_V3(player_velocity, physics3d->velocity.y), velocity_to_add);

	reset_after_touch(&ball_entity->ball);
}

void entity_ball_shoot(struct entity *ball_entity,
					   entity_id_t kicker_entity_id,
					   v2 direction,
					   f32 height,
					   f32 power,
					   b32 chip,
					   struct game_state *game_state)
{
	// struct component_transform *transform = &entity->transform;
	struct component_physics3d *physics3d = &ball_entity->physics3d;
	struct component_ball *ball = &ball_entity->ball;
	struct component_collider *collider = &ball_entity->collider;
	struct component_transform *transform = &ball_entity->transform;

	v3 shot_direction;
	f32 shot_force;
	if (chip) {
		shot_direction = Vector3Normalize(V2_TO_V3(
			direction,
			math_ext_split_lerp(game_state->constants.ball_chip_shot_height, 0.2f, height)));
		shot_force = math_ext_split_lerp(game_state->constants.ball_chip_shot_force, 0.5f, power);
	} else {
		shot_direction = Vector3Normalize(V2_TO_V3(
			direction, math_ext_split_lerp(game_state->constants.ball_shot_height, 0.2f, height)));
		shot_force = math_ext_split_lerp(game_state->constants.ball_shot_force, 0.5f, power);
	}

	f32 ball_radius = collider->colliders[0].props.circle.radius;
	if (!ball->is_on_the_ground) {
		if (transform->virtual_height > (ball_radius + 12)) {
			// TODO: this needs to handle headers
			GLOG_DEBUG_CTX(LS_GAME, ball_entity->header.name, "ball shot, should be an header");
		} else if (transform->virtual_height > ball_radius) {
			if (physics3d->velocity.y < 0) {
				// Ball is falling
				shot_force *= 1.05f;
				GLOG_DEBUG_CTX(
					LS_GAME, ball_entity->header.name, "ball shot during fall trajectory");
			} else if (physics3d->velocity.y > 0) {
				// If it's not falling then maybe it's a drop shot?
				if (IS_TIME_PASSED_IN_RANGE(game_state->time, ball->last_bounce_time, 0, 0.25f)) {
					// Drop shot
					shot_force *= 1.10;
					GLOG_DEBUG_CTX(LS_GAME, ball_entity->header.name, "ball drop shot");
				} else if (IS_TIME_PASSED_IN_RANGE(
							   game_state->time, ball->last_bounce_time, 0.25f, 0.5f)) {
					// Nope, this must be the shittiest shot ever
					shot_force *= 0.8f;
					GLOG_DEBUG_CTX(LS_GAME, ball_entity->header.name, "ball drop shot too late");
				}
			}
		}
	}

	if (game_state->constants.shoot_aid_no_inertia) {
		// reset velocity to avoid existing inertia affecting the trajectory: not sure this is
		// better though, need to test
		physics3d->velocity = Vector3Zero();
	}

	physics3d->accel =
		Vector3Add(physics3d->accel, Vector3Scale(shot_direction, shot_force / game_state->dt));
	ball->last_kick_time = game_state->time;
	ball->last_kicker_entity_id = kicker_entity_id;

	set_after_touch(&ball_entity->ball);
}

void entity_ball_short_pass(struct entity *ball_entity,
							entity_id_t kicker_entity_id,
							v3 direction,
							f32 power,
							b32 auto_pass,
							struct game_state *game_state)
{
	struct component_physics3d *physics3d = &ball_entity->physics3d;
	struct component_ball *ball = &ball_entity->ball;

	// reset velocity to avoid existing inertia affecting the trajectory
	physics3d->velocity = Vector3Zero();
	power =
		math_ext_split_lerp(game_state->constants.ball_pass_force, auto_pass ? 0.5f : 0.3f, power);

	physics3d->accel =
		Vector3Add(physics3d->accel, Vector3Scale(direction, power / game_state->dt));
	ball->last_kick_time = game_state->time;
	ball->last_kicker_entity_id = kicker_entity_id;
}

void entity_ball_long_pass(struct entity *ball_entity,
						   entity_id_t kicker_entity_id,
						   v2 direction,
						   f32 power,
						   b32 driven,
						   struct game_state *game_state)
{
	struct component_physics3d *physics3d = &ball_entity->physics3d;
	struct component_ball *ball = &ball_entity->ball;

	v3 long_pass_direction;
	f32 long_pass_force;
	if (driven) {
		long_pass_direction = Vector3Normalize(
			V2_TO_V3(direction, game_state->constants.ball_long_driven_pass_height));
		long_pass_force =
			math_ext_split_lerp(game_state->constants.ball_long_driven_pass_force, 0.5f, power);
	} else {
		long_pass_direction =
			Vector3Normalize(V2_TO_V3(direction, game_state->constants.ball_long_pass_height));
		long_pass_force =
			math_ext_split_lerp(game_state->constants.ball_long_pass_force, 0.5f, power);
	}

	// reset velocity to avoid existing inertia affecting the trajectory
	physics3d->velocity = Vector3Zero();

	physics3d->accel = Vector3Add(
		physics3d->accel, Vector3Scale(long_pass_direction, long_pass_force / game_state->dt));

	ball->last_kick_time = game_state->time;
	ball->last_kicker_entity_id = kicker_entity_id;
}

void entity_ball_tackle(struct entity *ball_entity,
						entity_id_t tackler_entity_id,
						v3 direction,
						f32 power,
						struct game_state *game_state)
{
	struct component_physics3d *physics3d = &ball_entity->physics3d;
	struct component_ball *ball = &ball_entity->ball;

	// physics3d->velocity = Vector3Zero();
	physics3d->accel = Vector3Add(
		physics3d->accel,
		Vector3Scale(direction,
					 math_ext_split_lerp(game_state->constants.ball_tackle_force, 0.5f, power) /
						 game_state->dt));
	ball->last_tackle_time = game_state->time;
	ball->last_tackle_entity_id = tackler_entity_id;
	ball->last_kick_time = game_state->time;
	ball->last_kicker_entity_id = tackler_entity_id;

	reset_after_touch(&ball_entity->ball);
}

void entity_ball_header(struct entity *ball_entity,
						entity_id_t header_entity_id,
						v3 direction,
						f32 force,
						struct game_state *game_state)
{
	struct component_physics3d *physics3d = &ball_entity->physics3d;
	struct component_ball *ball = &ball_entity->ball;

	physics3d->velocity = Vector3Zero();
	physics3d->accel =
		Vector3Add(physics3d->accel, Vector3Scale(direction, force / game_state->dt));
	ball->last_kick_time = game_state->time;
	ball->last_kicker_entity_id = header_entity_id;

	reset_after_touch(&ball_entity->ball);
}

void entity_ball_deviate_trajectory(struct entity *ball_entity,
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

	reset_after_touch(&ball_entity->ball);
}

void entity_ball_held_by_goalie(struct entity *ball_entity,
								entity_id_t goalie_entity_id,
								struct game_state *game_state)
{
	struct component_ball *ball = &ball_entity->ball;

	entity_ball_set_last_touch(ball, goalie_entity_id, game_state);

	state_machine_ball_change_state_to(&ball->state_machine, BS_GK_HANDS, game_state);

	reset_after_touch(&ball_entity->ball);
}

void entity_ball_saved_by_goalie(struct entity *ball_entity,
								 entity_id_t goalie_entity_id,
								 struct game_state *game_state)
{
	struct component_ball *ball = &ball_entity->ball;

	entity_ball_set_last_touch(ball, goalie_entity_id, game_state);

	reset_after_touch(&ball_entity->ball);
}

void entity_ball_back_in_play(struct entity *ball_entity, struct game_state *game_state)
{
	struct component_ball *ball = &ball_entity->ball;

	state_machine_ball_change_state_to(&ball->state_machine, BS_IN_PLAY, game_state);
}

entity_id_t entity_ball_player_in_possession(struct entity *ball_entity,
											 struct game_state *game_state)
{
	// printf("lteid: %d - fpt: %d - ltm: %d - ltt>lkt: %d\n",
	// 	   ball_entity->ball.last_touch_entity_id != EID_NOT_VALID,
	// 	   HAS_TIME_PASSED(game_state->time, ball_entity->ball.first_player_possession_time, 0.1f),
	// 	   !HAS_TIME_PASSED(game_state->time, ball_entity->ball.last_touch_time, 1.0f),
	// 	   ball_entity->ball.last_touch_time > ball_entity->ball.last_kick_time);
	if (ball_entity->ball.last_touch_entity_id != EID_NOT_VALID &&
		HAS_TIME_PASSED(game_state->time, ball_entity->ball.first_player_possession_time, 0.1f) &&
		!HAS_TIME_PASSED(game_state->time, ball_entity->ball.last_touch_time, 1.0f) &&
		ball_entity->ball.last_touch_time > ball_entity->ball.last_kick_time) {
		return ball_entity->ball.last_touch_entity_id;
	} else {
		return EID_NOT_VALID;
	}
}

// Team possession is total: whoever touched the ball last still owns it, for the whole
// flight of a pass or a shot, until someone from the other team touches it.
enum team_side entity_ball_team_in_possession(struct entity *ball_entity,
											  struct game_state *game_state)
{
	struct entity *last_touch_entity =
		entity_get(game_state->entity_repository, ball_entity->ball.last_touch_entity_id);
	if (!last_touch_entity) {
		// TODO: no last touch means a set piece is pending, ask the game manager which
		// team is taking it instead of defaulting.
		return TS_HOME;
	}
	return last_touch_entity->player.team_side;
}

entity_id_t entity_ball_player_on_the_ball(struct entity *ball_entity,
										   struct game_state *game_state)
{
	if (ball_entity->ball.last_touch_entity_id != EID_NOT_VALID &&
		ball_entity->ball.last_touch_time == game_state->time) {
		return ball_entity->ball.last_touch_entity_id;
	} else {
		return EID_NOT_VALID;
	}
}

// The ball model has no closed form for travel time: quadratic drag, a regime switch
// between gravity and rolling resistance at ground level, and bouncing. So step the real
// integrator on a throwaway copy of the ball state.
fg_time_t entity_ball_time_to_cover_distance(struct entity *ball_entity,
											 v3 kick_velocity,
											 f32 distance,
											 struct game_state *game_state)
{
	struct component_transform *transform = &ball_entity->transform;
	const v2 start_position = transform->position;
	const f32 mass = ball_entity->physics3d.mass;
	const f32 radius = ball_entity->collider.colliders[0].props.circle.radius;
	const u32 max_steps = (u32)(BALL_PASS_MAX_TRAVEL_TIME / game_state->dt);

	v3 position = V2_TO_V3(start_position, transform->virtual_height);
	v3 velocity = kick_velocity;
	b32 on_the_ground;
	b32 bounced;
	for (u32 step = 1; step <= max_steps; ++step) {
		physics_ball_simulation(&position,
								&velocity,
								Vector3Zero(),
								mass,
								radius,
								Vector3Zero(),
								&on_the_ground,
								&bounced,
								game_state);

		if (Vector2Distance(V3_TO_V2(position), start_position) >= distance) {
			return step * game_state->dt;
		}
		// Ball came to rest short of the distance
		if (Vector2Length(V3_TO_V2(velocity)) < 1.0f) {
			return step * game_state->dt;
		}
	}

	return BALL_PASS_MAX_TRAVEL_TIME;
}

static b32 is_applied_after_touch_same_side(v2 stored_side, v2 selected_side)
{
	f32 projection = Vector2ProjectScalar(stored_side, selected_side);
	if (projection > 0) {
		return true;
	}

	return false;
}

v3 entity_ball_after_touch_acceleration(struct component_ball *ball,
										v2 direction,
										v3 ball_velocity,
										fg_time_t time,
										struct game_state *game_state)
{
	// TODO: we'd like CPU controlled players to also be able to control after touch in some way.
	v3 after_touch_accel = Vector3Zero();
	f32 after_touch_time = (time - ball->last_kick_time);
	if (after_touch_time > game_state->constants.ball_after_touch_delay_time) {
		v2 after_touch_side = Vector2Zero();
		b32 apply_after_touch = true;

		f32 after_touch_pressure =
			Vector2ProjectExt(ball->after_touch.activation_direction, direction, &after_touch_side);

		if (ball->after_touch.selected_side.x == 0 && ball->after_touch.selected_side.y == 0) {
			if (fabsf(after_touch_pressure) > 0.25f) {
				// After touch side not selected yet
				// We need some after touch to be actually applied
				ball->after_touch.selected_side = after_touch_side;
			} else {
				apply_after_touch = false;
			}
		} else if (!is_applied_after_touch_same_side(ball->after_touch.selected_side,
													 after_touch_side)) {
			// If player changed side of after touch we keep applying the first one
			// we've seen
			after_touch_side = ball->after_touch.selected_side;
		}

		if (apply_after_touch) {
			// Extract signed swerve amount from after_touch_side
			f32 after_touch_amount =
				Vector2ProjectScalar(ball->after_touch.activation_direction, after_touch_side);

			// Apply force perpendicular to the ball's current velocity
			v2 ball_velocity_2d = V3_TO_V2(ball_velocity);
			v2 force_dir_2d =
				Vector2Scale(Vector2Perp(Vector2Normalize(ball_velocity_2d)), after_touch_amount);
			v3 after_touch_direction = V2_TO_V3(force_dir_2d, 0);

			// After touch force decays exponentially with time.
			// Formula is:
			// 1 - clamp((time_since_ball_kicked - after_touch_delay)^2 /
			// (max_after_touch_time - after_touch_delay)^2, 0, 1)
			f32 force =
				1 -
				Clamp(powf((after_touch_time - game_state->constants.ball_after_touch_delay_time),
						   2) /
						  powf((game_state->constants.ball_after_touch_max_time -
								game_state->constants.ball_after_touch_delay_time),
							   2),
					  0,
					  1);

			// Apply it
			after_touch_accel = Vector3Scale(after_touch_direction,
											 game_state->constants.ball_after_touch_force * force);
		}
	} else {
		if (ball->after_touch.activation_direction.x == 0 &&
			ball->after_touch.activation_direction.y == 0) {
			ball->after_touch.activation_direction = Vector2Perp(direction);
		}
	}
	return after_touch_accel;
}

static void set_after_touch(struct component_ball *ball)
{
	ball->after_touch.active = true;
	ball->after_touch.selected_side = ball->after_touch.activation_direction = Vector2Zero();
}

static void reset_after_touch(struct component_ball *ball)
{
	ball->after_touch.active = false;
	ball->after_touch.selected_side = ball->after_touch.activation_direction = Vector2Zero();
}
