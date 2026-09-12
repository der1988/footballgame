#include "system.h"
#include "entity_components.h"
#include "entity_player.h"
#include "entity.h"
#include "entity_ball.h"
#include "football.h"
#include "game.h"
#include "input.h"
#include "physics.h"
#include "math_ext.h"
#include "render.h"
#include "sprite.h"
#include "types.h"
#include <raymath.h>

static void apply_future_position(struct future_position *future_position, v3 position, u32 dts);

void system_ball(struct game_state *game_state)
{
	struct entity *ball_entity = game_state->vars.ball.entity;

	DECLARE_ENTITY_COMPONENT(ball_entity, transform);
	DECLARE_ENTITY_COMPONENT(ball_entity, physics3d);
	DECLARE_ENTITY_COMPONENT(ball_entity, sprite);
	DECLARE_ENTITY_COMPONENT(ball_entity, ball);
	DECLARE_ENTITY_COMPONENT(ball_entity, collider);

	struct state_machine_ball *state_machine = &ball->state_machine;

	const u8 game_controller_idx = 0;
	struct game_controller *gc = &game_state->input_manager->game_controllers[game_controller_idx];

	if (gc->state.current[GAME_CONTROL_TEST_RESET].digital.pressed) {
		struct entity *p1 = entity_get(game_state->entity_repository,
									   game_state->human_players[0].controlled_player.entity_id);
		// Nobody is controlled until selection first runs.
		if (!p1) {
			return;
		}
		struct component_transform *p1_transform = &p1->transform;
		entity_ball_set_position_and_velocity(
			ball_entity,
			V2_TO_V3(Vector2Add(p1_transform->position, (v2){.y = -20}), 50),
			Vector3Zero());
		return;
	}

	// Test cross-bar
	if (gc->state.current[GAME_CONTROL_TEST_CROSSBAR].digital.pressed) {
		entity_ball_set_position_and_velocity(ball_entity,
											  (v3){.x = game_state->vars.pitch.field.x + 340,
												   .z = 300},
											  Vector3Scale((v3){.x = 0, .y = 1, .z = -1}, 290.5f));
		return;
	}

	// Test cross
	if (gc->state.current[GAME_CONTROL_TEST_CROSS].digital.pressed) {
		entity_ball_set_position_and_velocity(
			ball_entity,
			(v3){.x = game_state->vars.pitch.field.x + 600, .z = 100},
			Vector3Scale((v3){.x = -1, .y = 1.0f, .z = 0.2}, 280.5f));
		return;
	}

	// Test post
	if (gc->state.current[GAME_CONTROL_TEST_POST].digital.pressed) {
		entity_ball_set_position_and_velocity(ball_entity,
											  (v3){.x = game_state->vars.pitch.field.x + 465.97,
												   .z = 1050},
											  Vector3Scale((v3){.x = -1, .y = 1.67, .z = 1}, 260));
		return;
	}

	// Test goalie jump
	if (gc->state.current[GAME_CONTROL_TEST_GOALIE_JUMP].digital.pressed) {
		entity_ball_set_position_and_velocity(
			ball_entity,
			(v3){.x = game_state->vars.pitch.goal[PSI_NORTH].penalty_spot.x,
				 .y = 200,
				 .z = game_state->vars.pitch.goal[PSI_NORTH].penalty_spot.y},
			Vector3Zero());
		return;
	}

	// Sprite draw ordering when ball is in upper goal
	sprite->layer = transform->position.y < game_state->vars.pitch.field.y ? SL_NORTH_GOAL :
																			 SL_PLAYERS_AND_BALL;

	switch (state_machine->current_state) {
	case BS_NONE:
		break;
	case BS_IN_PLAY: {
		v3 position = V2_TO_V3(transform->position, transform->virtual_height);
		v3 velocity = physics3d->velocity;
		v3 accel = physics3d->accel;

		struct future_position *future_bounce = &ball->future_positions[FUTURE_POSITION_BOUNCE];
		struct future_position *future_reachable_to_player =
			&ball->future_positions[FUTURE_POSITION_REACHABLE_TO_PLAYER];
		struct future_position *future_reachable_to_goalie =
			&ball->future_positions[FUTURE_POSITION_REACHABLE_TO_GOALIE];

		// After touch
		struct human_player *human_player =
			game_input_human_player_by_player_entity_id(game_state, ball->last_play_entity_id);
		v2 after_touch_direction = Vector2Zero();
		v3 after_touch_accel = Vector3Zero();
		if (human_player && ball->after_touch.active) {
			struct game_controller *game_controller =
				&game_state->input_manager->game_controllers[human_player->game_controller_id];
			after_touch_direction = input_movement_vector(game_controller);
			after_touch_accel = entity_ball_after_touch_acceleration(
				ball, after_touch_direction, physics3d->velocity, game_state->time, game_state);
		}

		b32 on_the_ground;
		b32 bounced;
		f32 ball_radius = collider->colliders[0].props.circle.radius;
		physics_ball_simulation(&position,
								&velocity,
								accel,
								physics3d->mass,
								ball_radius,
								after_touch_accel,
								&on_the_ground,
								&bounced,
								game_state);

		/* Reset future positions. */
		future_bounce->available = false;
		future_reachable_to_player->available = false;
		future_reachable_to_goalie->available = false;

		ball->is_on_the_ground = on_the_ground && !bounced;
		if (bounced) {
			ball->last_bounce_time = game_state->time;
		}
		if (ball->is_on_the_ground) {
			apply_future_position(future_bounce, position, 0);
			game_state->vars.hud.show_crosshair = false;
		}
		if (position.y < HEADER_MAX_HEIGHT) {
			apply_future_position(future_reachable_to_player, position, 0);
		} else {
			game_state->vars.hud.show_crosshair = true;
		}
		if (position.y < game_state->constants.goalie_max_reachable_height) {
			apply_future_position(future_reachable_to_goalie, position, 0);
		}

		// Calculate next 'ball_lookaheads' look ahead positions, ball_lookahead_dts dts apart from each other
		b32 lookahead_on_the_ground;
		b32 lookahead_bounced;
		v3 lookahead_position = position;
		v3 lookahead_velocity = velocity;
		u16 lookaheads = game_state->constants.ball_lookaheads;
		u32 lookahead_dts = game_state->constants.ball_lookahead_dts;
		for (u16 i = 0; i < lookaheads; ++i) {
			for (u16 j = 0; j < lookahead_dts; ++j) {
				u32 dts = i * lookahead_dts + j + 1;

				// We calculate after touch acceleration at this future point in time
				v3 after_touch_accel =
					entity_ball_after_touch_acceleration(ball,
														 after_touch_direction,
														 lookahead_velocity,
														 game_state->time + dts * game_state->dt,
														 game_state);

				physics_ball_simulation(&lookahead_position,
										&lookahead_velocity,
										Vector3Zero(),
										physics3d->mass,
										ball_radius,
										after_touch_accel,
										&lookahead_on_the_ground,
										&lookahead_bounced,
										game_state);
				if (!future_bounce->available && lookahead_bounced) {
					apply_future_position(future_bounce, lookahead_position, dts);
				}
				if (!future_reachable_to_player->available &&
					lookahead_position.y < HEADER_MAX_HEIGHT) {
					apply_future_position(future_reachable_to_player, lookahead_position, dts);
				}
				if (!future_reachable_to_goalie->available &&
					lookahead_position.y < game_state->constants.goalie_max_reachable_height) {
					apply_future_position(future_reachable_to_goalie, lookahead_position, dts);
				}
			}
			game_state->vars.ball.lookahead_positions[i] = lookahead_position;
		}

		// Project position to 2D
		transform->position = V3_TO_V2(position);
		transform->virtual_height = position.y;

		physics3d->velocity = velocity;
		// Acceleration does not accumulate
		physics3d->accel = Vector3Zero();

		// Calculate frame to display
		v2 ball_velocity = V3_TO_V2(physics3d->velocity);
		if (Vector2Length(ball_velocity) > 3) {
			enum sprite_orientation sprite_orientation =
				sprite_calculate_orientation(ball_velocity);
			struct asset_animation *animation =
				asset_animation_get_by_id(game_state->assets.animation_cache,
										  game_state->gfx.ball.roll_animations[sprite_orientation]);
			f32 ball_max_velocity = game_state->constants.ball_max_velocity / 10.0f;
			f32 delay = (animation->delay *
						 (5 - 4 * Clamp(Vector2Length(ball_velocity) / ball_max_velocity, 0, 1)));
			if ((game_state->time - ball->last_frame_update_time) > delay) {
				ball->last_frame_update_time = game_state->time;
				u16 new_frame_id = (ball->last_running_frame_idx + 1) % animation->frames_count;
				sprite->frame_id = animation->frames[new_frame_id];
				ball->last_running_frame_idx = new_frame_id;
			}
			sprite->orientation = sprite_orientation;
		}

		// Handle ball sprite layer
		if (transform->virtual_height > game_state->constants.crossbar_height + ball_radius) {
			// If ball is very high it's going to be over everything
			sprite->layer = SL_FOREGROUND;
		} else if (game_state->gameplay.state_machine.current_state == FS_GOAL) {
			// If ball is in goal then it depends on which goal
			if (transform->position.y < game_state->vars.pitch.field.y) {
				sprite->layer = SL_BACKGROUND;
			} else {
				sprite->layer = SL_FOREGROUND;
			}
		} else if (transform->position.y >
				   (game_state->vars.pitch.field.y + game_state->vars.pitch.field.height)) {
			// Ball is not in goal and it's out of the pitch on the south side
			sprite->layer = SL_FOREGROUND;
		} else {
			sprite->layer = SL_PLAYERS_AND_BALL;
		}
	} break;
	case BS_OUT_OF_PLAY: {
		break;
	}
	case BS_GK_HANDS: {
		break;
	}
	case BS_COUNT:
		break;
	}
}

static void apply_future_position(struct future_position *future_position, v3 position, u32 dts)
{
	future_position->available = true;
	future_position->position = position;
	future_position->dts = dts;
}
