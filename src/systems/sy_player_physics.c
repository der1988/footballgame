#include "components.h"
#include "entities.h"
#include "e_ball.h"
#include "e_player.h"
#include "football.h"
#include "game.h"
#include "input.h"
#include "macro.h"
#include <math_ext.h>
#include <raymath.h>
#include <stdio.h>
#include "ai.h"
#include "types.h"

void systems_player_physics(struct game_state *game_state)
{
	struct entity *ball_entity = game_state->vars.ball_entity;

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

		const struct player_derived_physics dp =
			entity_player_derive_physics(&player->characteristics);

		collider->colliders[PCT_PLAYER].props.circle.radius =
			game_state->constants.player_control_radius;

		switch (state_machine->current_state) {
		case PS_NORMAL: {
			v2 effective_steering = player_controls->steering;

			// If charging kick we seek the ball
			struct human_player *human_player =
				game_input_get_human_player_by_player_entity_id(game_state, entity_id);
			if (human_player) {
				struct game_controller *gc =
					&game_state->input_manager->game_controllers[human_player->game_controller_id];
				if (gc->charge_lock.state == INPUT_CHARGE_LOCK_CHARGING ||
					gc->charge_lock.state == INPUT_CHARGE_LOCK_CHARGED) {
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
						f32 max_angle_cos = 0.85f; // ~30 degrees

						if (dot > max_angle_cos) {
							// Blend strength: stronger when closer to ball
							f32 proximity = 1.0f - (dist_to_ball * dist_to_ball) /
													   (magnet_radius * magnet_radius);
							f32 blend = 1.0f * proximity;
							effective_steering =
								Vector2Normalize(Vector2Lerp(steering_dir, dir_to_ball, blend));
						}
					}
				}
			}

			v2 force_to_apply = Vector2Zero();
			v2 normalized_velocity = Vector2Normalize(physics->velocity);
			f32 steering_magnitude = Vector2Length(effective_steering);
			f32 current_velocity = Vector2Length(physics->velocity);

			// Run force (scaled by body type)
			force_to_apply = Vector2Scale(effective_steering,
										  game_state->constants.player_movement_force *
											  dp.movement_force_multiplier);

			// Apply drag if no steering applied (scaled by body type)
			if (steering_magnitude < 0.01f) {
				if (current_velocity < 3) {
					physics->velocity = Vector2Zero();
				} else {
					Vector2 drag = Vector2Scale(Vector2Negate(normalized_velocity),
												game_state->constants.player_movement_drag *
													dp.drag_multiplier);
					force_to_apply = Vector2Add(force_to_apply, drag);
				}
			}

			// Handle max velocity (scaled by body type)
			f32 player_run_max_velocity =
				game_state->constants.player_run_max_velocity * dp.max_velocity_multiplier;
			f32 initial_max_velocity = player_run_max_velocity * 0.5f;
			physics->max_velocity = initial_max_velocity;

			// Stop: ramp down max velocity toward zero
			if (player_controls->stop) {
				player->current_max_velocity -= 0.75f * dp.decel_ramp_down * game_state->dt;
				player->current_max_velocity = fmaxf(0, player->current_max_velocity);
			} else {
				if (player->current_max_velocity < initial_max_velocity) {
					// Recover from zero when stop is released and player steers
					player->current_max_velocity = initial_max_velocity;
				}

				f32 projection = Vector2ProjectScalar(physics->velocity, effective_steering);
				if (projection > 0 && player->current_max_velocity < player_run_max_velocity) {
					player->current_max_velocity += dp.accel_ramp_up * game_state->dt;
					player->current_max_velocity =
						fminf(player->current_max_velocity, player_run_max_velocity);
				} else if (current_velocity < player->current_max_velocity) {
					player->current_max_velocity -= dp.decel_ramp_down * game_state->dt;
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
			entity_player_anim_calculate_running_frame(entity,
													   game_state->gfx.player.stand_frames,
													   game_state->gfx.player.run_animations,
													   physics->velocity,
													   3,
													   game_state);

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
				static f32 delay_by_phase[] = {0, 0.05f, 0.10f, 0.15f, 0.2f};
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
			// 4 -> HP_MAX_ELEVATION
			static u16 animation_frames[] = {0, 0, 0, 1, 1, 2, 2, 3, 3, 4};
			header_state_data->phase = animation_frames[new_frame_id];

			v2 force_to_apply = Vector2Zero();

			// Handle max velocity
			physics->max_velocity = game_state->constants.player_run_max_velocity;

			entity_player_physics_apply_force(entity, force_to_apply, game_state);
			entity_player_physics_apply_velocity(entity, game_state);

			if (header_state_data->animation_completed) {
				entity_player_state_machine_change_state_to(
					&player->state_machine, PS_NORMAL, game_state);
				break;
			}

			break;
		}
		case PS_SLIDING_TACKLE: {
			struct state_machine_player_sliding_tackle_data *sliding_tackle_state_data =
				&player->state_machine.state_data.sliding_tackle;

			v2 force_to_apply = Vector2Zero();
			if (Vector2Length(physics->velocity) > 3) {
				// Apply friction
				force_to_apply = Vector2Scale(Vector2Negate(Vector2Normalize(physics->velocity)),
											  7500); // TODO: this should depend on type of pitch
			} else if (HAS_TIME_PASSED(
						   game_state->time, sliding_tackle_state_data->start_time, 0.5f)) {
				entity_player_state_machine_change_state_to(
					&player->state_machine, PS_NORMAL, game_state);
				break;
			}

			// Handle max velocity
			physics->max_velocity = game_state->constants.player_run_max_velocity;

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
				// Apply friction
				force_to_apply = Vector2Scale(Vector2Negate(Vector2Normalize(physics->velocity)),
											  30000); // TODO: this should depend on type of pitch
			}

			// Handle max velocity
			physics->max_velocity = game_state->constants.player_run_max_velocity;

			entity_player_physics_apply_force(entity, force_to_apply, game_state);
			entity_player_physics_apply_velocity(entity, game_state);

			if (fall_state_data->animation_completed &&
				HAS_TIME_PASSED(game_state->time, fall_state_data->animation_end_time, 1.f)) {
				entity_player_state_machine_change_state_to(
					&player->state_machine, PS_NORMAL, game_state);
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
