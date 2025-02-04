#include "e_player.h"
#include "assets.h"
#include "components.h"
#include "entities.h"
#include "football.h"
#include "input.h"
#include "log.h"
#include "math_ext.h"
#include "game.h"
#include "memory.h"
#include "types.h"
#include "sprite.h"
#include "physics.h"
#include "collision.h"
#include "gfx.h"
#include <assert.h>
#include <raylib.h>
#include <string.h>

implement_state_machine_functions(player);
implement_state_machine_functions(goalie);

void player_custom_begin_draw(struct entity *entity, struct game_state *game_state);
void player_custom_after_draw(struct entity *entity, struct game_state *game_state);

struct entity *entities_player_create(struct player_config config, struct game_state *game_state)
{
	struct entity *entity =
		entities_entity_init(game_state->entity_repository, ET_PLAYER, "player_%02d");

	// Initialise transform
	struct component_transform *transform = &entity->transform;
	transform->header.enabled = true;
	transform->position = config.position;
	transform->rotation = 0;
	transform->scale = (v2){.x = 1, .y = 1};

	// Initialise physics
	struct component_physics *physics = &entity->physics;
	physics->header.enabled = true;
	physics->mass = 75.0f;
	physics->velocity = (Vector2){0};
	physics->accel = (Vector2){0};
	physics->max_velocity = game_state->constants.player_control_max_velocity;

	// Initialise collider
	struct component_collider *collider = &entity->collider;
	collider->header.enabled = true;

	// Initialise sprite
	char *player_frame_name = "player_front_0";
	struct assets_sprite_frame *sprite_frame =
		assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, player_frame_name);
	assert(sprite_frame != NULL);
	struct component_sprite *sprite = &entity->sprite;
	sprite->header.enabled = true;
	sprite->frame_id = sprite_frame->id;
	sprite->color = WHITE;
	sprite->flipX = false;
	sprite->flipY = false;
	sprite->pivot_override = (v2){.x = 0.5, .y = 0.65};
	sprite->layer = SL_PLAYERS_AND_BALL;
	sprite->draw_shadow = false;
	// sprite->shadow_color = BLACK;
	// sprite->shadow_color.a = 100;
	// sprite->shadow_offset = (v2){.x = 0.5, .y = 0.5};
	// sprite->shadow_scale = (v2){.x = 0.6, .y = 1};

	// Initialise player
	struct component_player *player = &entity->player;
	player->header.enabled = true;
	player->team_side = config.team_side;
	player->team_formation_position_idx = config.team_formation_position_idx;
	// Sets initial max velocity to half of max run velocity
	player->current_max_velocity = game_state->constants.player_run_max_velocity * 0.5f;
	struct state_machine_player *player_state_machine =
		memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(*player_state_machine));
	player_state_machine->owner_entity = entity->header.id;
	player_state_machine->previous_state = PS_NONE;
	player->state_machine = player_state_machine;

	// Initialise action
	struct component_action *action = &entity->action;
	action->header.enabled = true;

	// Initialise player controls
	struct component_player_controls *player_controls = &entity->player_controls;
	player_controls->header.enabled = true;

	if (config.is_goalie) {
		// Initialise goalie
		struct component_goalie *goalie = &entity->goalie;
		goalie->header.enabled = true;
		struct state_machine_goalie *state_machine =
			memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(*state_machine));
		state_machine->owner_entity = entity->header.id;
		state_machine->previous_state = GS_NONE;
		state_machine->current_state = GS_PROTECT_GOAL;
		goalie->state_machine = state_machine;
		player_state_machine->current_state = PS_GOALIE;

		entities_goalie_state_machine_on_enter(entity, game_state);
	} else {
		player_state_machine->current_state = PS_NORMAL;

		sprite->before_draw = &player_custom_begin_draw;
		sprite->after_draw = &player_custom_after_draw;
	}

	enum pitch_side pitch_side = game_state->vars.teams[config.team_side].play_context.pitch_side;

	collider->num_colliders = PCT_COUNT;
	collider->colliders = memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL],
											sizeof(struct collider) * collider->num_colliders);
	collider->colliders[PCT_GOALIE].shape = CS_CIRCLE;
	collider->colliders[PCT_GOALIE].props.circle.offset = (v2){.y = 0};
	collider->colliders[PCT_GOALIE].props.circle.radius = 6;

	collider->colliders[PCT_DIVE].shape = CS_RECTANGLE;
	collider->colliders[PCT_DIVE].props.rectangle.size.x = 22;
	collider->colliders[PCT_DIVE].props.rectangle.size.y = 10;
	collider->colliders[PCT_DIVE].props.rectangle.offset =
		(v2){.x = 0,
			 .y = 0 + (collider->colliders[PCT_DIVE].props.rectangle.size.y * 0.5f *
					   (pitch_side == PSI_NORTH ? -1 : 1))};

	collider->colliders[PCT_PLAYER].shape = CS_CIRCLE;
	collider->colliders[PCT_PLAYER].props.circle.radius =
		game_state->constants.player_control_radius;

	collider->colliders[PCT_TACKLE].shape = CS_CIRCLE;
	collider->colliders[PCT_TACKLE].props.circle.radius = 5;

	entities_player_state_machine_on_enter(entity, game_state);

	GLOG_INFO_CTX(LS_ENTITIES, entity->header.name, "player created");

	return entity;
}

void entities_player_apply_force(struct entity *player_entity,
								 Vector2 force_to_apply,
								 struct game_state *game_state)
{
	struct component_physics *physics = &player_entity->physics;

	physics->accel = Vector2Scale(force_to_apply, 1 / physics->mass);

	// Calculate velocity
	physics->velocity = Vector2Add(physics->velocity, Vector2Scale(physics->accel, game_state->dt));
	if (Vector2Length(physics->velocity) > physics->max_velocity) {
		physics->velocity =
			Vector2Scale(Vector2Normalize(physics->velocity), physics->max_velocity);
	}
}

void entities_player_apply_velocity(struct entity *player_entity, struct game_state *game_state)
{
	struct component_physics *physics = &player_entity->physics;
	struct component_transform *transform = &player_entity->transform;
	transform->position =
		Vector2Add(transform->position, Vector2Scale(physics->velocity, game_state->dt));
}

void entities_player_calculate_running_frame(struct entity *player_entity,
											 assets_sprite_frame_id_t *stand_frames,
											 assets_animation_id_t *run_animations,
											 v2 direction,
											 f32 velocity_threshold,
											 struct game_state *game_state)
{
	struct component_physics *physics = &player_entity->physics;
	struct component_player *player = &player_entity->player;
	struct component_sprite *sprite = &player_entity->sprite;

	f32 player_speed = Vector2Length(physics->velocity);
	if (Vector2Length(physics->velocity) > velocity_threshold) {
		enum sprite_orientation sprite_orientation = sprite_calculate_orientation(direction);
		struct assets_animation *animation = assets_animation_get_by_id(
			game_state->assets.animation_cache, run_animations[sprite_orientation]);

		f32 max_player_velocity = game_state->constants.player_run_max_velocity;
		f32 player_speed_ratio = player_speed / max_player_velocity;

		if ((game_state->time - player->last_frame_update_time) >
			(animation->delay * (2.5f - player_speed_ratio))) {
			player->last_frame_update_time = game_state->time;
			u16 new_frame_id = (player->last_running_frame_idx + 1) % animation->frames_count;
			sprite->frame_id = animation->frames[new_frame_id];
			player->last_running_frame_idx = new_frame_id;
		}
		sprite->orientation = sprite_orientation;
	} else {
		sprite->frame_id = stand_frames[sprite->orientation];
	}
}

void entities_player_state_machine_on_enter(struct entity *entity, struct game_state *game_state)
{
	UNUSED(game_state);

	struct component_player *player = &entity->player;
	struct component_sprite *sprite = &entity->sprite;
	struct state_machine_player *state_machine = player->state_machine;

	GLOGF_INFO_CTX(LS_ENTITIES,
				   entity->header.name,
				   "entering state: %s",
				   state_machine_player_state_string[state_machine->current_state]);

	// Reset state data
	memset(&state_machine->state_data, 0, sizeof(union state_machine_player_state_data));

	switch (state_machine->current_state) {
	case PS_NONE:
	case PS_NORMAL:
	case PS_GOALIE:
	case PS_THROW_IN:
	case PS_HEADER:
	case PS_SLIDING_TACKLE:
	case PS_FALL:
		game_input_player_charge_reset(game_state, entity->header.id);
		struct entity *ball_entity =
			entities_entity_get(game_state->entity_repository, game_state->vars.ball_entity_id);
		if (ball_entity->ball.last_touch_entity_id == entity->header.id) {
			// Falling player had the ball, so let's reset touch time to make it clear player is no longer in possession
			ball_entity->ball.last_touch_time = 0;
		}
		entity->sprite.frame_id = game_state->gfx.player.tackle_frames[sprite->orientation];
		entity->player.state_machine->state_data.tackle.start_time = game_state->time;
		break;
	case PS_COUNT:
		break;
	}
}

void entities_player_state_machine_on_exit(struct entity *entity, struct game_state *game_state)
{
	UNUSED(game_state);

	struct state_machine_player *state_machine = entity->player.state_machine;

	GLOGF_INFO_CTX(LS_ENTITIES,
				   entity->header.name,
				   "exiting state: %s",
				   state_machine_player_state_string[state_machine->previous_state]);

	switch (state_machine->previous_state) {
	case PS_NONE:
	case PS_NORMAL:
	case PS_GOALIE:
	case PS_THROW_IN:
	case PS_HEADER:
	case PS_SLIDING_TACKLE:
	case PS_FALL:
	case PS_COUNT:
		break;
	}
}

void entities_goalie_state_machine_on_enter(struct entity *entity, struct game_state *game_state)
{
	struct state_machine_goalie *state_machine = entity->goalie.state_machine;

	GLOGF_INFO_CTX(LS_ENTITIES,
				   entity->header.name,
				   "entering state: %s",
				   state_machine_goalie_state_string[state_machine->current_state]);

	// Reset state data
	memset(&state_machine->state_data, 0, sizeof(union state_machine_goalie_state_data));

	switch (state_machine->current_state) {
	case GS_NONE:
		break;
	case GS_PROTECT_GOAL: {
		entity->physics.max_velocity = game_state->constants.player_run_max_velocity * 1.25f;
		break;
	}
	case GS_DIVE: {
		entity->physics.max_velocity =
			entity->goalie.dive_speed * game_state->constants.goalie_dive_max_velocity;
		state_machine->state_data.dive.animation_start_time = game_state->time;
		break;
	}
	case GS_HOLD_BALL: {
		entity->physics.velocity = Vector2Zero();
		entity->physics.accel = Vector2Zero();
		entity->goalie.state_machine->state_data.hold_ball.hold_start_time = game_state->time;
		entity->sprite.frame_id =
			game_state->gfx.goalie.save_frames[game_state->vars.teams[entity->player.team_side]
												   .play_context.pitch_side][ST_WITH_BALL];
		break;
	}
	case GS_JUMP: {
		entity->physics.velocity = Vector2Zero();
		entity->physics.accel = Vector2Zero();
		state_machine->state_data.jump.animation_start_time = game_state->time;
		break;
	}
	case GS_COUNT:
		break;
	}
}

void entities_goalie_state_machine_on_exit(struct entity *entity, struct game_state *game_state)
{
	struct state_machine_goalie *state_machine = entity->goalie.state_machine;

	GLOGF_INFO_CTX(LS_ENTITIES,
				   entity->header.name,
				   "exiting state: %s",
				   state_machine_goalie_state_string[state_machine->previous_state]);

	switch (state_machine->previous_state) {
	case GS_NONE:
		break;
	case GS_PROTECT_GOAL: {
		break;
	}
	case GS_DIVE: {
		entity->physics.max_velocity = game_state->constants.player_run_max_velocity;
		break;
	}
	case GS_HOLD_BALL: {
		break;
	}
	case GS_JUMP: {
		break;
	}
	case GS_COUNT:
		break;
	}
}

b32 entities_player_is_colliding_with_ball(struct entity *entity,
										   enum player_collider_type collider_type,
										   v3 ball_position,
										   f32 ball_radius,
										   f32 max_height)
{
	const v2 player_position = entity->transform.position;
	const v2 ball_pos = V3_TO_V2(ball_position);
	b32 check_collision = false;
	struct collider *collider = &entity->collider.colliders[collider_type];

	switch (collider_type) {
	case PCT_DIVE:
		check_collision = CheckCollisionCircleRec(
			ball_pos,
			ball_radius,
			calculate_collision_rectangle(
				player_position, collider->props.rectangle.size, collider->props.rectangle.offset));
		break;
	case PCT_TACKLE:
		collider->props.circle.offset =
			ORIENTATION_TO_TACKLE_COLLIDER_OFFSET[entity->sprite.orientation];
	case PCT_PLAYER:
	case PCT_GOALIE:
		check_collision =
			CheckCollisionCircles(ball_pos,
								  ball_radius,
								  Vector2Add(player_position, collider->props.circle.offset),
								  collider->props.circle.radius);
		break;
	case PCT_COUNT:
		break;
	}

	return check_collision && ball_position.y < max_height;
}

b32 entities_player_should_dive(struct entity *goalie_entity,
								struct entity *ball_entity,
								enum save_direction *dive_direction,
								enum save_height *dive_height,
								f32 *dive_speed,
								f32 *dive_distance,
								struct game_state *game_state)
{
	b32 should_dive = false;
	const f32 ball_radius = ball_entity->collider.colliders[0].props.circle.radius;

	const u32 steps_per_lookahead =
		(game_state->vars.slow_motion ? SLOW_MOTION_LOOKAHEAD_MULTIPLIER : 1) *
		game_state->constants.ball_lookahead_dts;
	// We want a lookahead of 150ms for our goalie
	const int lookaheads =
		floor(Clamp((GOALIE_LOOKAHEAD_MS / game_state->constants.fixed_timestep) /
						game_state->constants.ball_lookahead_dts,
					1,
					game_state->constants.ball_lookaheads));
	for (u16 i = 0; i < lookaheads; ++i) {
		v3 pos_to_check = ball_entity->ball.lookahead_positions[i];

		const b32 is_colliding_with_ball =
			entities_player_is_colliding_with_ball(goalie_entity,
												   PCT_GOALIE,
												   pos_to_check,
												   ball_radius,
												   game_state->constants.crossbar_height);

		if (is_colliding_with_ball) {
			// Ball is coming our direction, let's wait
			break;
		}
		// Check the ball is still savable
		f32 x_diff = pos_to_check.x - goalie_entity->transform.position.x;
		f32 y_diff = pos_to_check.z - goalie_entity->transform.position.y;
		f32 distance_to_cover = fabsf(x_diff);
		if (HAS_TIME_PASSED(game_state->time,
							ball_entity->ball.last_kick_time,
							game_state->constants.goalie_dive_reaction_time) &&
			y_diff > 0 && y_diff < 5 && distance_to_cover < 60 &&
			pos_to_check.y < game_state->constants.crossbar_height) {
			f32 time_to_unsaveable = (i + 1) * game_state->dt * steps_per_lookahead;
			f32 max_distance_we_can_cover_in_available_time =
				game_state->constants.goalie_dive_max_velocity * time_to_unsaveable / 1.0f;
			*dive_direction = x_diff > 0 ? SD_RIGHT : SD_LEFT;
			*dive_height = pos_to_check.y < GOALIE_DIVE_HEIGHT_THRESHOLD ? SH_LOW : SH_HIGH;
			*dive_speed =
				Clamp(distance_to_cover / max_distance_we_can_cover_in_available_time, 0, 1);
			*dive_distance = distance_to_cover;
			should_dive = true;
			GLOGF_DEBUG_CTX(LS_GAME,
							goalie_entity->header.name,
							"goalie has %.3fs to save a ball that will be %.2fpx away: "
							"dive speed %.2fpx/s",
							time_to_unsaveable,
							distance_to_cover,
							goalie_entity->goalie.dive_speed *
								game_state->constants.goalie_dive_max_velocity);
			break;
		}
	}
	return should_dive;
}

void entities_player_gained_possession(struct entity *player_entity, struct game_state *game_state)
{
	player_entity->player.last_possession_time = game_state->time;
}

enum player_ball_possession entities_player_ball_possession(struct entity *player_entity,
															struct entity *ball_entity,
															struct game_state *game_state)
{
	if (player_entity->header.id == ball_entity->ball.last_touch_entity_id) {
		if (ball_entity->ball.last_touch_time == game_state->time) {
			return PBP_ON_THE_BALL;
		} else {
			return PBP_IN_POSSESSION;
		}
	} else {
		return PBP_OFF_THE_BALL;
	}
}

void entities_player_selected_for_pass(struct entity *player_entity, struct game_state *game_state)
{
	player_entity->player.last_selected_for_pass_time = game_state->time;
}

struct player_ball_reachability_prediction entities_player_can_reach_future_ball_position(
	struct entity *seeker_entity,
	f32 seeker_max_speed,
	v3 *future_ball_positions,
	b32(predicate)(v3 pos_to_check, struct entity *seeker_entity, struct game_state *game_state),
	struct game_state *game_state)
{
	struct player_ball_reachability_prediction prediction = {.can_reach_it = false};

	const int steps_per_lookahead =
		(game_state->vars.slow_motion ? SLOW_MOTION_LOOKAHEAD_MULTIPLIER : 1) *
		game_state->constants.ball_lookahead_dts;
	const int lookaheads = game_state->constants.ball_lookaheads;
	for (u16 i = 0; i < lookaheads; ++i) {
		v3 pos_to_check = future_ball_positions[i];
		if (!predicate || predicate(pos_to_check, seeker_entity, game_state)) {
			time_t lookahead_time = (i + 1) * game_state->dt * steps_per_lookahead;
			if (physics_can_reach_position_in_time(seeker_entity->transform.position,
												   V3_TO_V2(pos_to_check),
												   lookahead_time,
												   seeker_max_speed)) {
				prediction.can_reach_it = true;
				prediction.future_pos = pos_to_check;
				prediction.future_time = lookahead_time;
				return prediction;
			}
		}
	}
	return prediction;
}

struct entity *entities_player_find_pass_target(struct entity *player_on_the_ball_entity,
												f32 detection_angle,
												f32 max_distance,
												struct game_state *game_state)
{
	b32 is_player_human_controlled = false;
	for (usize i = 0; i < MAX_HUMAN_PLAYERS; ++i) {
		if (game_state->human_players[i].controlled_player_entity_id ==
			player_on_the_ball_entity->header.id) {
			is_player_human_controlled = true;
			break;
		}
	}
	struct entity_repository *entity_repository = game_state->entity_repository;
	struct entity *selected_entity = NULL;
	f32 selected_score = 0;
	struct entity *entity;
	for (usize entity_id = 0; entity_id < entity_repository->max_slots; ++entity_id) {
		entity = entities_entity_get(game_state->entity_repository, entity_id);
		if (entity->header.alive && entity->header.type == ET_PLAYER) {

			struct component_transform *transform = &entity->transform;

			if (player_on_the_ball_entity->header.id == entity_id ||
				player_on_the_ball_entity->player.team_side != entity->player.team_side) {
				// We don't want to pass to ourselves nor to the other team, right?
				continue;
			}

			v2 selected_direction = player_on_the_ball_entity->player_controls.steering;
			v2 to_player =
				Vector2Subtract(transform->position, player_on_the_ball_entity->transform.position);
			f32 distance_from_player_sqr = Vector2LengthSqr(to_player);
			f32 angle = fabs(Vector2Angle(selected_direction, to_player) * RAD2DEG);
			time_low_prec_t last_time_selected_for_pass =
				entity->player.last_selected_for_pass_time;
			b32 nearer_to_goal = game_state->vars.teams[player_on_the_ball_entity->player.team_side]
											 .play_context.pitch_side == PSI_NORTH ?
									 (entity->transform.position.y >
									  player_on_the_ball_entity->transform.position.y) :
									 (entity->transform.position.y <
									  player_on_the_ball_entity->transform.position.y);

			f32 score = 0;
			if (!is_player_human_controlled) {
				score = 3 * (1 - Clamp01(distance_from_player_sqr / (max_distance * max_distance)));
				score += 2 * (1 - Clamp01(angle / detection_angle));
				score += 5 * Clamp01((game_state->time - last_time_selected_for_pass) / 1.f);
				score += 4 * nearer_to_goal;
				score /= 14;
			} else {
				score = 1 * (1 - Clamp01(distance_from_player_sqr / (max_distance * max_distance)));
				score += 2 * (1 - Clamp01(angle / detection_angle));
				score /= 3;
			}

			if (score > selected_score) {
				selected_entity = entity;
				selected_score = score;

				GLOGF_TRACE(LS_GAME, "found valid pass target: %zu, %.2f", entity_id, score);
			}
		}
	}
	if (selected_entity != NULL) {
		entities_player_selected_for_pass(selected_entity, game_state);
		GLOGF_TRACE(LS_GAME,
					"selected pass target %zu with score %.2f",
					selected_entity->header.id,
					selected_score);
	}
	return selected_entity;
}

entity_id_t entities_player_find_nearest_teammate(entity_id_t player_to_not_select_entity_id,
												  v2 position,
												  enum team_side team_side,
												  struct game_state *game_state)
{
	struct entity *ball_entity =
		entities_entity_get(game_state->entity_repository, game_state->vars.ball_entity_id);
	struct component_ball *ball = &ball_entity->ball;

	struct entity_repository *entity_repository = game_state->entity_repository;
	entity_id_t selected_entity_id = EID_NOT_VALID;
	f32 selected_distance = UINT32_MAX;
	struct entity *entity;
	for (usize entity_id = 0; entity_id < entity_repository->max_slots; ++entity_id) {
		entity = entities_entity_get(game_state->entity_repository, entity_id);
		if (entity->header.alive && entity->header.type == ET_PLAYER) {
			struct component_transform *transform = &entity->transform;
			struct component_goalie *goalie = &entity->goalie;

			if (player_to_not_select_entity_id == entity_id ||
				entity->player.team_side != team_side ||
				(goalie->header.enabled && goalie->state_machine->current_state != GS_HOLD_BALL) ||
				ball->pass_target_entity_id == entity_id) {
				// If player is the one on the ball or a goalie or the destination of the pass let's
				// skip
				continue;
			}

			f32 distance = Vector2Distance(position, transform->position);
			if (distance < selected_distance) {
				selected_entity_id = entity_id;
				selected_distance = distance;
			}
		}
	}
	return selected_entity_id;
}

struct entity *entities_player_find_nearest_facing_opponent(struct entity *player_entity,
															struct game_state *game_state)
{
	struct entity_repository *entity_repository = game_state->entity_repository;
	struct entity *opponent_entity;
	for (usize opponent_entity_id = 0; opponent_entity_id < entity_repository->max_slots;
		 ++opponent_entity_id) {
		opponent_entity = entities_entity_get(game_state->entity_repository, opponent_entity_id);
		if (opponent_entity->header.alive && opponent_entity->header.type == ET_PLAYER) {
			struct component_transform *transform = &opponent_entity->transform;

			if (player_entity->header.id == opponent_entity_id ||
				opponent_entity->player.team_side == player_entity->player.team_side) {
				// If player is in our team or the player we are considering we skip
				continue;
			}
			v2 to_opponent = Vector2Subtract(opponent_entity->transform.position,
											 player_entity->transform.position);
			f32 angle = fabs(Vector2Angle(player_entity->physics.velocity, to_opponent) * RAD2DEG);

			f32 distance = Vector2Distance(player_entity->transform.position, transform->position);
			if (distance < 100 && angle < 30) {
				return opponent_entity;
			}
		}
	}
	return NULL;
}

struct entities entities_player_find_tackled_opponents(struct entity *player_entity,
													   struct game_state *game_state)
{
	const u8 max_tackled_players = 3;
	struct entities tackled_players = {
		.length = 0,
		.array = memory_arena_push(game_state->memory_by_lifetime[MAL_UPDATE],
								   sizeof(struct entity *) * max_tackled_players)};
	struct entity_repository *entity_repository = game_state->entity_repository;
	struct entity *opponent_entity;
	u8 current_idx = 0;
	for (usize opponent_entity_id = 0; opponent_entity_id < entity_repository->max_slots;
		 ++opponent_entity_id) {
		opponent_entity = entities_entity_get(game_state->entity_repository, opponent_entity_id);
		if (opponent_entity->header.alive && opponent_entity->header.type == ET_PLAYER &&
			opponent_entity->player.state_machine->current_state == PS_NORMAL &&
			opponent_entity->player.team_side != player_entity->player.team_side) {

			const v2 opponent_collider_pos =
				Vector2Add(opponent_entity->transform.position,
						   opponent_entity->collider.colliders[PCT_PLAYER].props.circle.offset);
			const v2 player_collider_pos =
				Vector2Add(player_entity->transform.position,
						   player_entity->collider.colliders[PCT_TACKLE].props.circle.offset);

			const b32 is_tackled = CheckCollisionCircles(
				player_collider_pos,
				player_entity->collider.colliders[PCT_TACKLE].props.circle.radius,
				opponent_collider_pos,
				opponent_entity->collider.colliders[PCT_PLAYER].props.circle.radius);

			if (is_tackled) {
				tackled_players.array[current_idx++] = opponent_entity;
				tackled_players.length++;
				if (tackled_players.length == max_tackled_players) {
					break;
				}
			}
		}
	}
	return tackled_players;
}

void player_custom_begin_draw(struct entity *entity, struct game_state *game_state)
{
	u32 *palette =
		memory_arena_push(game_state->memory_by_lifetime[MAL_UPDATE], sizeof(u32) * 3 * 10);

	color_t kit_color = entity->player.team_side == TS_HOME ? RED : BLUE;

	palette = (u32[]){
		// Hair
		0,
		0,
		0,
		// Skin
		201,
		147,
		120,
		// Skin shadow
		166,
		111,
		90,
		// Kit
		kit_color.r,
		kit_color.g,
		kit_color.b,
		// Kit shadow
		201,
		147,
		120,
		// Pantaloncini
		255,
		255,
		255,
		// Shoes
		0,
		0,
		0,
	};

	struct assets_shader *assets_shader =
		assets_shader_get_by_name(game_state->assets.shader_cache, "shader_palette");
	u32 location_idx = GetShaderLocation(assets_shader->shader, "palette");
	SetShaderValueV(assets_shader->shader, location_idx, palette, SHADER_UNIFORM_IVEC3, 7);

	BeginShaderMode(assets_shader->shader);
}

void player_custom_after_draw(struct entity *entity, struct game_state *game_state)
{
	UNUSED(entity);
	UNUSED(game_state);

	EndShaderMode();
}
