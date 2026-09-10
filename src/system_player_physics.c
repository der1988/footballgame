#include "system.h"
#include "entity_components.h"
#include "entity.h"
#include "entity_ball.h"
#include "entity_player.h"
#include "football.h"
#include "game.h"
#include "input.h"
#include "macro.h"
#include "math_ext.h"
#include "physics.h"
#include <raymath.h>
#include <stdio.h>
#include "ai.h"
#include "types.h"
#include "constants.h"

void system_player_physics(struct game_state *game_state)
{
	struct entity *ball_entity = game_state->vars.ball.entity;

	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
	UNUSED(ball_physics_3d);
	UNUSED(ball);
	UNUSED(ball_collider);

	const entity_id_t player_in_possession_entity_id =
		entity_ball_player_in_possession(ball_entity, game_state);

	FOR_EACH_ENTITY(entity, game_state->entity_repository)
	{
		if (!entity_is_alive_and_has_components(
				entity, 5, C_TRANSFORM, C_PHYSICS, C_SPRITE, C_PLAYER, C_COLLIDER)) {
			continue;
		}

		DECLARE_ENTITY_COMPONENT(entity, transform);
		DECLARE_ENTITY_COMPONENT(entity, physics);
		DECLARE_ENTITY_COMPONENT(entity, player);
		DECLARE_ENTITY_COMPONENT(entity, collider);
		DECLARE_ENTITY_COMPONENT(entity, sprite);

		struct state_machine_player *state_machine = &player->state_machine;

		struct component_player_controls *player_controls = &entity->player_controls;

		collider->colliders[PCT_PLAYER].props.circle.radius = player->constants.control_radius;

		switch (state_machine->current_state) {
		case PS_NORMAL: {
			v2 effective_steering = player_controls->steering;
			b32 charging_kick = false;

			// If charging kick we seek the ball
			struct human_player *human_player =
				game_input_human_player_by_player_entity_id(game_state, entity_id);
			if (human_player) {
				struct game_controller *gc =
					&game_state->input_manager->game_controllers[human_player->game_controller_id];
				if (input_charge_lock_is_held(gc)) {
					charging_kick = true;
					effective_steering = Vector2Normalize(
						Vector2Subtract(ball_transform->position, transform->position));
				}
				// Ball magnetism: gently bend steering toward the ball when
				// the player is already heading roughly in the right direction
				else if (Vector2Length(effective_steering) > 0.3f &&
						 player_in_possession_entity_id != entity_id) {
					v2 to_ball = Vector2Subtract(ball_transform->position, transform->position);
					f32 dist_to_ball = Vector2Length(to_ball);
					f32 magnet_radius = 80.0f;

					if (dist_to_ball > 1.0f && dist_to_ball < magnet_radius) {
						v2 dir_to_ball = Vector2Scale(to_ball, 1.0f / dist_to_ball);
						v2 steering_dir = Vector2Normalize(effective_steering);
						f32 dot = Vector2DotProduct(steering_dir, dir_to_ball);

						// Widen angle tolerance at close range (60° close, 30° far)
						f32 t = dist_to_ball / magnet_radius;
						f32 max_angle_cos = 0.5f + 0.35f * t;

						if (dot > max_angle_cos) {
							// Use existing ball lookahead to find interception point
							struct player_ball_reachability_prediction prediction =
								entity_player_can_reach_future_ball_position(
									entity,
									physics->max_velocity,
									game_state->vars.ball.lookahead_positions,
									NULL,
									game_state);
							v2 target_pos = prediction.can_reach_it ?
												V3_TO_V2(prediction.future_pos) :
												ball_transform->position;

							v2 dir_to_target =
								Vector2Normalize(Vector2Subtract(target_pos, transform->position));

							// Blend: player input far away, interception direction close
							f32 proximity = 1.0f - (dist_to_ball * dist_to_ball) /
													   (magnet_radius * magnet_radius);
							effective_steering = Vector2Normalize(
								Vector2Lerp(steering_dir, dir_to_target, proximity));
						}
					}
				}
			}

			v2 force_to_apply = Vector2Zero();
			f32 steering_magnitude = Vector2Length(effective_steering);
			f32 current_velocity = Vector2Length(physics->velocity);

			// Run force (per-player)
			force_to_apply = Vector2Scale(effective_steering, player->constants.movement_force);

			// Apply drag if no steering applied (per-player)
			if (steering_magnitude < 0.01f && !physics_set_at_rest_if_needed(physics, 3)) {
				force_to_apply = Vector2Add(
					force_to_apply, physics_drag_force(physics->velocity, player->constants.drag));
			}

			// Handle max velocity (per-player)
			f32 player_run_max_velocity = player->constants.run_max_velocity;
			f32 initial_max_velocity = player_run_max_velocity * 0.5f;
			physics->max_velocity = initial_max_velocity;

			// Stop: ramp down max velocity toward zero
			if (player_controls->stop) {
				player->current_max_velocity -=
					0.75f * player->constants.decel_ramp_down * game_state->dt;
				player->current_max_velocity = fmaxf(0, player->current_max_velocity);
			} else {
				if (player->current_max_velocity < initial_max_velocity) {
					// Recover from zero when stop is released and player steers
					player->current_max_velocity = initial_max_velocity;
				}

				f32 projection = Vector2ProjectScalar(physics->velocity, effective_steering);
				if (projection > 0 && player->current_max_velocity < player_run_max_velocity) {
					player->current_max_velocity +=
						player->constants.accel_ramp_up * game_state->dt;
					player->current_max_velocity =
						fminf(player->current_max_velocity, player_run_max_velocity);
				} else if (current_velocity < player->current_max_velocity) {
					player->current_max_velocity -=
						player->constants.decel_ramp_down * game_state->dt;
					player->current_max_velocity =
						fmaxf(initial_max_velocity, player->current_max_velocity);
				}
			}

			// Player without the ball is 10% faster than the same player with the ball
			f32 possession_mult = player_in_possession_entity_id == entity_id ? 0.9f : 1.0f;
			physics->max_velocity = player->current_max_velocity * possession_mult;

			entity_player_physics_apply_force(entity, force_to_apply, game_state);
			entity_player_physics_apply_velocity(entity, game_state);

			// Calculate frame to display
			struct state_machine_player_normal_data *normal_state_data =
				&player->state_machine.state_data.normal;

			// A skid is the run drawn differently, never simulated differently: everything above
			// this point runs the same whether one is under way or not.
			if (normal_state_data->dribble.active) {
				v2 steering = normal_state_data->dribble.steering;

				sprite->frame_id =
					game_state->gfx.player.dribble_frames[normal_state_data->dribble.turn];

				b32 running_the_new_way = Vector2DotProduct(Vector2Normalize(physics->velocity),
															steering) >= DRIBBLE_END_ALIGNMENT;

				// Asking for a third heading abandons the turn the pose brakes, so the skid is
				// over whether or not the run ever came round. Steering let go is not a new
				// request: he is only coasting the turn out.
				v2 requested_steering = player_controls->steering;
				b32 asked_for_another_turn =
					!IS_V2_ZERO(requested_steering) &&
					Vector2DotProduct(Vector2Normalize(requested_steering), steering) <
						DRIBBLE_SAME_REQUEST_ALIGNMENT;

				b32 committed = !HAS_TIME_PASSED(game_state->time,
												 normal_state_data->dribble.start_time,
												 game_state->constants.player_dribble_min_time);
				// A charge taking over the run ends the skid at once, min_time or not: the
				// player is seeking the ball now, not turning.
				if (charging_kick ||
					(!committed && (running_the_new_way || asked_for_another_turn ||
									physics_is_at_rest(physics->velocity, 3))) ||
					HAS_TIME_PASSED(game_state->time,
									normal_state_data->dribble.start_time,
									game_state->constants.player_dribble_max_time)) {
					normal_state_data->dribble.active = false;
				}
			} else {
				entity_player_anim_calculate_running_frame(entity,
														   game_state->gfx.player.stand_frames,
														   game_state->gfx.player.run_animations,
														   physics->velocity,
														   3,
														   game_state);

				// Tested against what was asked for, never the assisted steering above: that one
				// is bent toward the ball, which is not a direction the player asked to turn on
				if (entity_player_should_dribble(entity, player_controls->steering, game_state)) {
					entity_player_dribble_start(entity, player_controls->steering, game_state);
				}
			}

			break;
		}
		case PS_HEADER: {
			struct state_machine_player_header_data *header_state_data =
				&player->state_machine.state_data.header;

			struct asset_animation *animation = asset_animation_get_by_id(
				game_state->assets.animation_cache,
				game_state->gfx.player.header_animations[sprite->orientation]);
			u16 new_frame_id = header_state_data->last_frame_id;
			// Elevation
			if (!header_state_data->elevation_completed) {
				f32 animation_completion =
					Clamp((game_state->time - header_state_data->animation_start_time) /
							  (animation->delay * animation->frames_count),
						  0,
						  1);
				new_frame_id = floorf(animation_completion * (animation->frames_count - 1));
				header_state_data->last_frame_id = new_frame_id;
				if (animation_completion >= 1) {
					player->state_machine.state_data.header.elevation_completed = true;
					player->state_machine.state_data.header.elevation_completion_time =
						game_state->time;
				}
				entity->sprite.frame_id = animation->frames[new_frame_id];
			}
			// Suspension
			else if (!header_state_data->should_fall_down) {
				static f32 delay_by_phase[] = {0, 0.04f, 0.08f, 0.12f, 0.16f, 0.2f};
				if (HAS_TIME_PASSED(game_state->time,
									header_state_data->elevation_completion_time,
									delay_by_phase[header_state_data->phase])) {
					header_state_data->max_elevation_frame_id = header_state_data->last_frame_id;
					header_state_data->should_fall_down = true;
					header_state_data->animation_start_time = game_state->time;
				}
			}
			// Landing
			else if (!header_state_data->animation_completed) {
				f32 animation_completion =
					Clamp((game_state->time - header_state_data->animation_start_time) /
							  (animation->delay * (header_state_data->max_elevation_frame_id + 1)),
						  0,
						  1);
				new_frame_id = floorf((1.0f - animation_completion) *
									  (header_state_data->max_elevation_frame_id + 1));
				header_state_data->last_frame_id = new_frame_id;
				if (animation_completion >= 1) {
					header_state_data->animation_completed = true;
				}
				entity->sprite.frame_id = animation->frames[new_frame_id];
			}

			// new_frame_id to header_phase:
			// 0 -> HP_GROUND
			// 1 -> HP_AIR_1
			// 1 -> HP_AIR_2
			// 1 -> HP_AIR_3
			// 4 -> HP_AIR_4
			// 5 -> HP_MAX_ELEVATION
			static u16 animation_frames[] = {0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5};
			header_state_data->phase = animation_frames[new_frame_id];

			// Apply friction
			// TODO: this needs to be tweaked
			v2 force_to_apply = Vector2Zero();
			if (!physics_set_at_rest_if_needed(physics, 1)) {
				force_to_apply = physics_drag_force(physics->velocity, 2000);
			}

			// Handle max velocity
			physics->max_velocity = player->constants.run_max_velocity;

			entity_player_physics_apply_force(entity, force_to_apply, game_state);
			entity_player_physics_apply_velocity(entity, game_state);

			if (header_state_data->animation_completed) {
				state_machine_player_fire(&player->state_machine, PT_RECOVER, game_state);
				break;
			}

			break;
		}
		case PS_TACKLE: {
			struct state_machine_player_tackle_data *tackle_state_data =
				&player->state_machine.state_data.tackle;

			const b32 at_rest = physics_set_at_rest_if_needed(physics, 3);

			// Body contact can hold a player above the rest threshold for as long as it lasts,
			// so the challenge also ends on time whether or not he has come to a stop.
			if ((at_rest &&
				 HAS_TIME_PASSED(game_state->time, tackle_state_data->start_time, 0.15f)) ||
				HAS_TIME_PASSED(game_state->time, tackle_state_data->start_time, 0.5f)) {
				state_machine_player_fire(&player->state_machine, PT_RECOVER, game_state);
				break;
			}

			v2 force_to_apply = Vector2Zero();
			if (!at_rest) {
				// Apply friction
				// TODO: this should depend on type of pitch
				force_to_apply =
					physics_drag_force(physics->velocity, player->constants.tackle_friction_force);
			}

			// Handle max velocity
			physics->max_velocity = player->constants.run_max_velocity;

			entity_player_physics_apply_force(entity, force_to_apply, game_state);
			entity_player_physics_apply_velocity(entity, game_state);

			break;
		}
		case PS_SLIDING_TACKLE: {
			struct state_machine_player_sliding_tackle_data *sliding_tackle_state_data =
				&player->state_machine.state_data.sliding_tackle;

			v2 force_to_apply = Vector2Zero();
			if (physics_set_at_rest_if_needed(physics, 3)) {
				if (HAS_TIME_PASSED(
						game_state->time, sliding_tackle_state_data->start_time, 0.25f)) {
					state_machine_player_fire(&player->state_machine, PT_RECOVER, game_state);
					break;
				}
			} else {
				// Apply friction
				// TODO: this should depend on type of pitch
				force_to_apply = physics_drag_force(
					physics->velocity, player->constants.sliding_tackle_friction_force);
			}

			// Handle max velocity
			physics->max_velocity = player->constants.run_max_velocity;

			entity_player_physics_apply_force(entity, force_to_apply, game_state);
			entity_player_physics_apply_velocity(entity, game_state);

			break;
		}
		case PS_FALL: {
			struct state_machine_player_fall_data *fall_state_data =
				&player->state_machine.state_data.fall;

			if (!fall_state_data->animation_completed) {
				struct asset_animation *animation = asset_animation_get_by_id(
					game_state->assets.animation_cache,
					game_state->gfx.player.fall_animations[sprite->orientation]);
				f32 animation_completion =
					Clamp((game_state->time - fall_state_data->animation_start_time) /
							  (animation->delay * animation->frames_count),
						  0,
						  1);
				const u16 new_frame_id =
					floorf(animation_completion * (animation->frames_count - 1));
				if (animation_completion >= 1) {
					animation_completion = 1;
					fall_state_data->animation_end_time = game_state->time;
					fall_state_data->animation_completed = true;
				}
				entity->sprite.frame_id = animation->frames[new_frame_id];
			}

			v2 force_to_apply = Vector2Zero();
			if (fall_state_data->animation_completed) {
				if (!physics_set_at_rest_if_needed(physics, 3)) {
					// Apply friction
					// TODO: this should depend on type of pitch
					force_to_apply = physics_drag_force(physics->velocity, 30000);
				}
			}

			// Handle max velocity
			physics->max_velocity = player->constants.run_max_velocity;

			entity_player_physics_apply_force(entity, force_to_apply, game_state);
			entity_player_physics_apply_velocity(entity, game_state);

			if (fall_state_data->animation_completed &&
				HAS_TIME_PASSED(game_state->time, fall_state_data->animation_end_time, 1.f)) {
				state_machine_player_fire(&player->state_machine, PT_RECOVER, game_state);
				break;
			}

			break;
		}
		case PS_NONE:
		case PS_GOALIE:
		case PS_THROW_IN:
		case PS_COUNT:
			break;
		}
	}
}
