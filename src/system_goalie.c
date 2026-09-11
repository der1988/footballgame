#include "system.h"
#include "entity_components.h"
#include "entity_ball.h"
#include "entity_player.h"
#include "entity.h"
#include "football.h"
#include "game.h"
#include "macro.h"
#include "math_ext.h"
#include "constants.h"
#include "physics.h"
#include "types.h"
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include "collision.h"

void system_goalie(struct game_state *game_state)
{
	struct entity *ball_entity = game_state->vars.ball.entity;
	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);

	FOR_EACH_ENTITY(entity, game_state->entity_repository)
	{
		if (!entity_is_alive_and_has_components(
				entity, 6, C_TRANSFORM, C_PHYSICS, C_SPRITE, C_PLAYER, C_GOALIE, C_COLLIDER)) {
			continue;
		}

		DECLARE_ENTITY_COMPONENT(entity, player);
		if (player->state_machine.current_state != PS_GOALIE)
			continue;
		DECLARE_ENTITY_COMPONENT(entity, transform);
		DECLARE_ENTITY_COMPONENT(entity, physics);
		DECLARE_ENTITY_COMPONENT(entity, sprite);
		DECLARE_ENTITY_COMPONENT(entity, goalie);
		DECLARE_ENTITY_COMPONENT(entity, collider);

		struct state_machine_goalie *state_machine = &goalie->state_machine;

		enum pitch_side pitch_side =
			game_state->vars.teams[player->team_side].play_context.pitch_side;
		const v2 goal_center = game_state->vars.pitch.goal[pitch_side].center;
		const f32 goal_width = game_state->vars.pitch.goal_width;

		const v3 ball_position = V2_TO_V3(ball_transform->position, ball_transform->virtual_height);
		const f32 ball_radius = ball_collider->colliders[0].props.circle.radius;
		const f32 ball_velocity = Vector3Length(ball_physics_3d->velocity);
		// const f32 ball_distance_from_goal = Vector2Distance(goal_center, V3_TO_V2(ball_position));

		struct collider *goalie_collider = &collider->colliders[PCT_GOALIE];

		entity_id_t player_in_possession_entity_id =
			entity_ball_player_in_possession(ball_entity, game_state);

		switch (state_machine->current_state) {
		case GS_NONE:
			break;
		case GS_PROTECT_GOAL: {
			struct collision_information collision_with_ball_info =
				entity_player_is_colliding_with_ball(entity,
													 PCT_GOALIE,
													 ball_position,
													 ball_entity->physics3d.velocity,
													 ball_radius,
													 game_state->constants.crossbar_height);
			if (collision_with_ball_info.is_colliding) {

				// Choose whether to bounce or hold the ball
				// Definitely hold if ball velocity is lower than 54km/h
				// Sometimes hold if ball velocity is lower than 108km/h
				// Otherwise bounce it
				const b32 hold = GOALIE_HOLD_OR_BOUNCE(ball_velocity);

				if (hold) {
					// Hold ball in hands
					entity_ball_held_by_goalie(ball_entity, entity_id, game_state);
					state_machine_goalie_fire(&goalie->state_machine, GT_CATCH, game_state);

				} else {
					// Bounce ball
					v3 new_ball_velocity = Vector3Reflect(ball_physics_3d->velocity, (v3){.z = 1});

					// Step ball position back one tick to place it outside the collider
					v3 where_to_place_ball = Vector3Add(
						V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
						Vector3Scale(Vector3Negate(ball_physics_3d->velocity), game_state->dt));

					entity_ball_set_position_and_velocity(
						ball_entity, where_to_place_ball, Vector3Scale(new_ball_velocity, 0.3f));
					entity_ball_saved_by_goalie(ball_entity, entity_id, game_state);

					// TODO: we should play some animation here and maybe transition to some stunned state
				}

				break;
			}

			// So we are not colliding with ball...

			v2 destination;
			v2 steering;
			f32 force;

			const f32 exit_max_speed = game_state->constants.player_run_max_velocity * 1.5f;

			struct future_position *future_reachable_position =
				&ball->future_positions[FUTURE_POSITION_REACHABLE_TO_GOALIE];

			// can we get the ball in our hands without moving by any chance?
			if (ball_transform->virtual_height >
					game_state->constants.goalie_max_reachable_height &&
				future_reachable_position->available && future_reachable_position->dts < 20 &&
				Vector2Distance(V3_TO_V2(future_reachable_position->position),
								transform->position) < 5) {
				state_machine_goalie_fire(state_machine, GT_JUMP, game_state);
				break;
			}

			// Ball future reachable position is different from current position only in case the ball is currently
			// not reachable by players because it's floating high in the air.
			v2 ball_future_reachable_position = V3_TO_V2(future_reachable_position->position);
			struct nearest_player nearest_opponent_to_the_ball = {0};
			const b32 found =
				entity_player_find_nearest(ball_future_reachable_position,
										   OPPONENT_TEAM_SIDE(entity->player.team_side),
										   LIST_ENTITY_IDS_EMPTY,
										   &nearest_opponent_to_the_ball,
										   game_state);

			b32 protect_goal = false;
			if ((future_reachable_position->available &&
				 football_pitch_is_position_inside_penalty_area(
					 ball_future_reachable_position,
					 game_state->vars.teams[entity->player.team_side].play_context.pitch_side,
					 game_state)) &&
				player_in_possession_entity_id == EID_NOT_VALID &&
				ball_velocity < (15 * PIXELS_PER_METER) &&
				(!found || nearest_opponent_to_the_ball.distance > 10)) {
				// We will chase the ball if all the following conditions are true:
				// - nearest opponent is reasonably far from the ball
				// - no player in possession of the ball
				// - ball is in the penalty box or ball future bounce is in the penalty box
				// - ball velocity is low

				// Calculate whether goalie can reach the ball at its next reachable position
				const b32 should_seek_ball_at_reachable_pos =
					future_reachable_position->available &&
					physics_can_reach_position_in_time(transform->position,
													   ball_future_reachable_position,
													   game_state->dt *
														   future_reachable_position->dts,
													   exit_max_speed);

				if (should_seek_ball_at_reachable_pos) {
					// seek ball future reachable position
					physics->max_velocity = exit_max_speed;
					force = game_state->constants.player_movement_force * 2;

					destination = ball_future_reachable_position;
					steering = entity_player_steer_arrive(entity, destination, game_state);
				} else {
					// Calculate whether goalie can reach the ball using predicted positions
					// At this point we already know no one is in possession
					const struct player_ball_reachability_prediction prediction =
						entity_player_can_reach_future_ball_position(
							entity,
							exit_max_speed,
							game_state->vars.ball.lookahead_positions,
							entity_player_goalie_can_claim_ball_at,
							game_state);

					if (prediction.can_reach_it) {
						physics->max_velocity = exit_max_speed;
						force = game_state->constants.player_movement_force * 1;

						destination = V3_TO_V2(prediction.future_pos);
						steering = entity_player_steer_seek(entity, destination);
					} else {
						// ball isn't within reach, let's protect goal
						protect_goal = true;
					}
				}
			} else {
				// Protect goal
				protect_goal = true;
			}

			if (protect_goal) {
				// Let's understand if we have to prepare for a dive
				b32 move_on_the_line = false;

				// We need to account for both pitch sides
				const b32 invert =
					game_state->vars.teams[entity->player.team_side].play_context.pitch_side ==
					PSI_NORTH;

				const f32 max_side_movement = goal_width * 0.5f * 0.7f;
				v2 left_pos = Vector2Add(
					goal_center, (v2){.x = -max_side_movement, .y = 20 * (invert ? 1 : -1)});
				v2 right_pos = Vector2Add(
					goal_center, (v2){.x = max_side_movement, .y = 20 * (invert ? 1 : -1)});

				for (u16 i = 0; i < game_state->constants.ball_lookaheads; ++i) {
					const v3 pos_to_check = game_state->vars.ball.lookahead_positions[i];

					v2 prev_pos;
					v2 new_pos;
					if (i == 0) {
						prev_pos = ball_entity->transform.position;
						new_pos = V3_TO_V2(pos_to_check);
					} else {
						prev_pos = V3_TO_V2(game_state->vars.ball.lookahead_positions[i - 1]);
						new_pos = V3_TO_V2(pos_to_check);
					}

					v2 intersection_point;
					if (collision_is_line_intersecting_line(
							prev_pos, new_pos, left_pos, right_pos, &intersection_point)) {
						physics->max_velocity =
							game_state->constants.player_run_max_velocity * 0.5f;
						force = game_state->constants.player_movement_force;
						destination = intersection_point;
						move_on_the_line = true;
						break;
					}
				}

				if (!move_on_the_line) {
					physics->max_velocity = game_state->constants.player_run_max_velocity;
					force = game_state->constants.player_movement_force;
					v2 direction =
						Vector2Normalize(Vector2Subtract(ball_transform->position, goal_center));
					destination = Vector2Add(goal_center, Vector2Scale(direction, 20));
				}

				steering = entity_player_steer_arrive(entity, destination, game_state);
			}

			// Run force
			const v2 force_to_apply = Vector2Scale(steering, force);

			entity_player_physics_apply_force(entity, force_to_apply, game_state);
			entity_player_physics_apply_velocity(entity, game_state);

			entity_player_anim_calculate_running_frame(entity,
													   game_state->gfx.goalie.stand_frames,
													   game_state->gfx.goalie.run_animations,
													   Vector2Subtract(ball_transform->position,
																	   transform->position),
													   5,
													   game_state);

			if (ball_velocity > game_state->constants.goalie_dive_min_ball_velocity &&
				entity_player_goalie_should_dive(entity,
												 ball_entity,
												 &goalie->dive_direction,
												 &goalie->dive_height,
												 &goalie->dive_speed,
												 &goalie->dive_distance,
												 game_state)) {
				// Only dive if ball velocity is higher than 36km/h
				state_machine_goalie_fire(&goalie->state_machine, GT_DIVE, game_state);
			}

			break;
		}
		case GS_JUMP: {
			struct state_machine_goalie_animation_data *jump_state_data =
				&goalie->state_machine.state_data.dive;

			if (!jump_state_data->animation_first_phase_completed) {
				struct asset_animation *animation = asset_animation_get_by_id(
					game_state->assets.animation_cache,
					game_state->gfx.goalie.jump_animations_start[pitch_side]);
				f32 animation_completion =
					Clamp((game_state->time - jump_state_data->animation_start_time) /
							  (animation->delay * animation->frames_count),
						  0,
						  1);
				const u16 new_frame_id =
					floorf(animation_completion * (animation->frames_count - 1));
				if (animation_completion >= 1) {
					animation_completion = 1;
					jump_state_data->animation_first_phase_completed = true;
					jump_state_data->animation_first_phase_end_time = game_state->time;
				}
				sprite->frame_id = animation->frames[new_frame_id];
			} else if ((jump_state_data->ball_held ||
						HAS_TIME_PASSED(game_state->time,
										jump_state_data->animation_first_phase_end_time,
										0.05f)) &&
					   !jump_state_data->animation_completed) {
				if (!jump_state_data->animation_second_phase_started) {
					jump_state_data->animation_second_phase_started = true;
					jump_state_data->animation_start_time = game_state->time;
				}
				struct asset_animation *animation = asset_animation_get_by_id(
					game_state->assets.animation_cache,
					game_state->gfx.goalie
						.jump_animations_end[pitch_side][jump_state_data->ball_held]);
				f32 animation_completion =
					Clamp((game_state->time - jump_state_data->animation_start_time) /
							  (animation->delay * animation->frames_count),
						  0,
						  1);
				const u16 new_frame_id =
					floorf(animation_completion * (animation->frames_count - 1));
				if (animation_completion >= 1) {
					animation_completion = 1;
					jump_state_data->animation_completed = true;
					jump_state_data->animation_end_time = game_state->time;
				}
				sprite->frame_id = animation->frames[new_frame_id];
			}

			const struct collision_information collision_with_ball_info =
				entity_player_is_colliding_with_ball(entity,
													 PCT_GOALIE,
													 ball_position,
													 ball_entity->physics3d.velocity,
													 ball_radius,
													 game_state->constants.crossbar_height);

			if (!jump_state_data->ball_saved &&
				HAS_TIME_PASSED(game_state->time, goalie->last_save_time, 0.1f) &&
				collision_with_ball_info.is_colliding && !jump_state_data->animation_completed) {
				// Choose whether to bounce or hold the ball
				// Definitely hold if ball velocity is lower than 54km/h
				// Sometimes hold if ball velocity is lower than 108km/h
				// Otherwise bounce it
				jump_state_data->ball_held = GOALIE_HOLD_OR_BOUNCE(ball_velocity);

				if (jump_state_data->ball_held) {
					entity_ball_held_by_goalie(ball_entity, entity_id, game_state);
				} else {
					// Bounce ball
					v3 new_ball_velocity =
						Vector3Normalize(Vector3Reflect(ball_physics_3d->velocity, (v3){.z = 1}));
					new_ball_velocity.y = 1;

					// Step ball position back one tick to place it outside the collider
					v3 where_to_place_ball = Vector3Add(
						V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
						Vector3Scale(Vector3Negate(ball_physics_3d->velocity), game_state->dt));

					entity_ball_set_position_and_velocity(ball_entity,
														  where_to_place_ball,
														  Vector3Scale(new_ball_velocity,
																	   10 * PIXELS_PER_METER));
					entity_ball_saved_by_goalie(ball_entity, entity_id, game_state);
				}

				goalie->last_save_time = game_state->time;
				jump_state_data->ball_saved = true;
			} else if (jump_state_data->animation_completed) {
				state_machine_goalie_fire(&goalie->state_machine,
										  jump_state_data->ball_held ? GT_CATCH : GT_RECOVER,
										  game_state);
			}
			break;
		}
		case GS_HOLD_BALL: {
			if (!HAS_TIME_PASSED(game_state->time,
								 goalie->state_machine.state_data.hold_ball.hold_start_time,
								 1)) {
				break;
			}
			entity_ball_back_in_play(ball_entity, game_state);
			i8 direction = pitch_side == PSI_NORTH ? 1 : -1;
			const v2 new_ball_position = Vector2Add(
				transform->position,
				Vector2Scale((v2){.y = direction},
							 goalie_collider->props.circle.offset.y +
								 goalie_collider->props.circle.radius + ball_radius + 7));
			entity_ball_set_position_and_velocity(
				ball_entity, V2_TO_V3(new_ball_position, 10), Vector3Zero());
			entity_ball_long_pass(
				ball_entity, entity_id, (v2){.y = direction}, 0.7f, false, game_state);
			state_machine_goalie_fire(&goalie->state_machine, GT_RELEASE, game_state);
			break;
		}
		case GS_DIVE: {
			enum save_direction direction = goalie->dive_direction;

			struct state_machine_goalie_animation_data *dive_state_data =
				&goalie->state_machine.state_data.dive;

			if (!dive_state_data->animation_first_phase_completed) {
				struct asset_animation *animation = asset_animation_get_by_id(
					game_state->assets.animation_cache,
					game_state->gfx.goalie
						.dive_animations_start[pitch_side][direction][goalie->dive_height]);
				f32 animation_completion =
					Clamp((game_state->time - dive_state_data->animation_start_time) /
							  (animation->delay * animation->frames_count),
						  0,
						  1);
				const u16 new_frame_id =
					floorf(animation_completion * (animation->frames_count - 1));
				if (animation_completion >= 1) {
					animation_completion = 1;
					dive_state_data->animation_first_phase_completed = true;
					dive_state_data->animation_first_phase_end_time = game_state->time;
				}
				sprite->frame_id = animation->frames[new_frame_id];
			} else if ((dive_state_data->ball_held ||
						HAS_TIME_PASSED(game_state->time,
										dive_state_data->animation_first_phase_end_time,
										Clamp(goalie->dive_speed * 0.35f, 0.25f, 0.35f))) &&
					   !dive_state_data->animation_completed) {
				if (!dive_state_data->animation_second_phase_started) {
					dive_state_data->animation_second_phase_started = true;
					dive_state_data->animation_start_time = game_state->time;
				}
				struct asset_animation *animation = asset_animation_get_by_id(
					game_state->assets.animation_cache,
					game_state->gfx.goalie
						.dive_animations_end[pitch_side][direction][goalie->dive_height]
											[dive_state_data->ball_held]);
				f32 animation_completion =
					Clamp((game_state->time - dive_state_data->animation_start_time) /
							  (animation->delay * animation->frames_count),
						  0,
						  1);
				const u16 new_frame_id =
					floorf(animation_completion * (animation->frames_count - 1));
				if (animation_completion >= 1) {
					animation_completion = 1;
					dive_state_data->animation_completed = true;
					dive_state_data->animation_end_time = game_state->time;
				}
				sprite->frame_id = animation->frames[new_frame_id];
			}

			v2 force_to_apply = Vector2Zero();
			if (dive_state_data->animation_completed) {
				if (!physics_set_at_rest_if_needed(physics, 5)) {
					// Apply friction
					// TODO: this should depend on type of pitch
					force_to_apply = physics_drag_force(
						physics->velocity, game_state->constants.goalie_dive_friction_force);
				}
			} else {
				v2 steering = direction == SD_LEFT ? Vector2Left() : Vector2Right();
				force_to_apply = Vector2Scale(steering, game_state->constants.goalie_dive_force);
			}
			entity_player_physics_apply_force(entity, force_to_apply, game_state);
			entity_player_physics_apply_velocity(entity, game_state);

			const struct collision_information collision_with_ball_info =
				entity_player_is_colliding_with_ball(entity,
													 PCT_DIVE,
													 ball_position,
													 ball_entity->physics3d.velocity,
													 ball_radius,
													 game_state->constants.crossbar_height -
														 ball_radius * 1.5f);

			if (!dive_state_data->ball_saved &&
				HAS_TIME_PASSED(game_state->time, goalie->last_save_time, 0.1f) &&
				collision_with_ball_info.is_colliding &&
				// If the ball hits the post and comes back we don't want to touch the ball
				((pitch_side == PSI_NORTH && ball_physics_3d->velocity.z < 0) ||
				 (pitch_side == PSI_SOUTH && ball_physics_3d->velocity.z > 0)) &&
				(!dive_state_data->animation_completed ||
				 // If ball height is very low we are going to make the save even when
				 // lying on the floor after the dive
				 ball_transform->virtual_height < ball_radius)) {

				// Choose whether to bounce or hold the ball
				// Definitely hold if ball velocity is lower than 54km/h
				// Sometimes hold if ball velocity is lower than 108km/h
				// Definitely don't hold the ball if ball is very high
				// Otherwise bounce it
				dive_state_data->ball_held = GOALIE_HOLD_OR_BOUNCE(ball_velocity) &&
											 ball_transform->virtual_height < 25;

				if (dive_state_data->ball_held) {
					entity_ball_held_by_goalie(ball_entity, entity_id, game_state);
				} else {
					// Bounce ball
					// Type of deflection depends on where the ball hit the diving goalkeeper

					f32 deflection_x = ball_transform->position.x - entity->transform.position.x;
					f32 dive_collider_width =
						entity->collider.colliders[PCT_DIVE].props.rectangle.size.x;
					f32 deflection_factor_x = Clamp1(deflection_x / (dive_collider_width * 0.5f));

					// Deflection velocity is scaled down from the impact velocity.
					// Minimum is 0.3x the original, max 0.5x.
					// If the ball is saved in the middle of the collider it's going to be near to 0.3x
					// If it's saved at the edges it's going to be near to 0.5x
					f32 velocity_scale =
						0.5f - (1 - deflection_factor_x * deflection_factor_x) * 0.2f;

					printf("defacx: %.2f\n", deflection_factor_x);

					v3 deflection =
						(v3){.x = 0, .y = 0, .z = ball_physics_3d->velocity.z > 0 ? -1 : 1};
					deflection =
						Vector3RotateByAxisAngle(deflection,
												 (v3){.y = pitch_side == PSI_SOUTH ? 1 : -1},
												 -deflection_factor_x * 90 * DEG2RAD);

					// Deflection over the bar
					if (ball_transform->virtual_height >= 25) {
						deflection =
							Vector3RotateByAxisAngle(deflection,
													 (v3){.x = pitch_side == PSI_SOUTH ? 1 : -1},
													 GetRandomValue(130, 170) * DEG2RAD);
						// Deflections over the bar keep most of the original velocity
						velocity_scale =
							0.9f - (1 - deflection_factor_x * deflection_factor_x) * 0.2f;
					}

					printf("velocity scale: %.2f\n", velocity_scale);

					deflection = Vector3Scale(
						deflection, Vector3Length(ball_physics_3d->velocity) * velocity_scale);

					// v3 new_ball_velocity = Vector3Reflect(ball_physics_3d->velocity, (v3){.z = 1});
					// new_ball_velocity = Vector3RotateByAxisAngle(
					//     new_ball_velocity, (v3){.y = 1},
					//     30 - 60 * GetRandomValue(0, 100) / 100.0f * DEG2RAD);

					// Step ball position back one tick to place it outside the collider
					v3 where_to_place_ball = Vector3Add(
						V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
						Vector3Scale(Vector3Negate(ball_physics_3d->velocity), game_state->dt));

					entity_ball_set_position_and_velocity(
						ball_entity, where_to_place_ball, deflection);
					entity_ball_saved_by_goalie(ball_entity, entity_id, game_state);
				}

				goalie->last_save_time = game_state->time;
				dive_state_data->ball_saved = true;
			} else if (dive_state_data->animation_completed &&
					   HAS_TIME_PASSED(game_state->time,
									   dive_state_data->animation_end_time,
									   game_state->constants.goalie_dive_recover_time)) {
				state_machine_goalie_fire(&goalie->state_machine,
										  dive_state_data->ball_held ? GT_CATCH : GT_RECOVER,
										  game_state);
			}
			break;
		}
		case GS_COUNT:
			break;
		}
	}
}
