#include "components.h"
#include "e_ball.h"
#include "e_player.h"
#include "entities.h"
#include "football.h"
#include "game.h"
#include "macro.h"
#include "steering.h"
#include "math_ext.h"
#include "constants.h"
#include "physics/physics.h"
#include "types.h"
#include <raymath.h>
#include <stdio.h>
#include "collision.h"

static b32 protect_goal_predicate(v3 pos_to_check,
								  struct entity *seeker_entity,
								  struct game_state *game_state)
{
	enum pitch_side side =
		game_state->vars.teams[seeker_entity->player.team_side].play_context.pitch_side;
	f32 ball_distance_from_goal =
		Vector3Distance(pos_to_check, V2_TO_V3(game_state->vars.pitch.goal[side].center, 0));
	return (
		football_pitch_is_position_inside_penalty_area(V3_TO_V2(pos_to_check), side, game_state) &&
		ball_distance_from_goal < 125 &&
		pos_to_check.y < (game_state->constants.crossbar_height - 7));
}

void systems_goalie(struct game_state *game_state, struct input *input)
{
	UNUSED(input);

	struct entity *ball_entity = game_state->vars.ball_entity;
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

		collider->colliders[PCT_PLAYER].props.circle.radius =
			game_state->constants.player_control_radius;
		struct collider *goalie_collider = &collider->colliders[PCT_GOALIE];

		entity_id_t player_in_possession_entity_id =
			entity_ball_player_in_possession(ball_entity, game_state);
		struct entity *player_in_possession_entity =
			entity_get(game_state->entity_repository, player_in_possession_entity_id);

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
					entity_goalie_state_machine_change_state_to(
						&goalie->state_machine, GS_HOLD_BALL, game_state);

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
			if (ball_transform->virtual_height > game_state->constants.goalie_reach_max_height &&
				future_reachable_position->available && future_reachable_position->dts < 20 &&
				Vector2Distance(V3_TO_V2(future_reachable_position->position),
								transform->position) < 5) {
				entity_goalie_state_machine_change_state_to(state_machine, GS_JUMP, game_state);
				break;
			}

			// Ball future reachable position is different from current position only in case the ball is currently
			// not reachable by players because it's floating high in the air.
			v2 ball_future_reachable_position = V3_TO_V2(future_reachable_position->position);
			struct nearest_player *nearest_opponent_to_the_ball =
				find_nearest_player(ball_future_reachable_position,
									OPPONENT_TEAM_SIDE(entity->player.team_side),
									LIST_ENTITY_IDS_EMPTY,
									game_state);

			b32 protect_goal = false;
			if ((future_reachable_position->available &&
				 football_pitch_is_position_inside_penalty_area(
					 ball_future_reachable_position,
					 game_state->vars.teams[entity->player.team_side].play_context.pitch_side,
					 game_state)) &&
				player_in_possession_entity_id == EID_NOT_VALID &&
				ball_velocity < (15 * PIXELS_PER_METER) &&
				nearest_opponent_to_the_ball->distance > 10) {
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
					steering = steering_arrive(transform->position,
											   physics->velocity,
											   physics->max_velocity,
											   destination,
											   10);
				} else {
					// Calculate whether goalie can reach the ball using predicted positions
					// At this point we already know no one is in possession
					const struct player_ball_reachability_prediction prediction =
						entity_player_can_reach_future_ball_position(entity,
																	 exit_max_speed,
																	 ball->lookahead_positions,
																	 protect_goal_predicate,
																	 game_state);

					if (prediction.can_reach_it) {
						physics->max_velocity = exit_max_speed;
						force = game_state->constants.player_movement_force * 1;

						destination = V3_TO_V2(prediction.future_pos);
						steering = steering_seek(transform->position,
												 physics->velocity,
												 physics->max_velocity,
												 destination);
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
					const v3 pos_to_check = ball_entity->ball.lookahead_positions[i];

					v2 prev_pos;
					v2 new_pos;
					if (i == 0) {
						prev_pos = ball_entity->transform.position;
						new_pos = V3_TO_V2(pos_to_check);
					} else {
						prev_pos = V3_TO_V2(ball_entity->ball.lookahead_positions[i - 1]);
						new_pos = V3_TO_V2(pos_to_check);
					}

					v2 intersection_point;
					if (is_line_intersecting_line(
							prev_pos, new_pos, left_pos, right_pos, &intersection_point)) {
						physics->max_velocity =
							game_state->constants.player_run_max_velocity * 0.35f;
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

				steering = steering_arrive(
					transform->position, physics->velocity, physics->max_velocity, destination, 10);
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
				entity_goalie_state_machine_change_state_to(
					&goalie->state_machine, GS_DIVE, game_state);
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
				if (jump_state_data->ball_held) {
					entity_goalie_state_machine_change_state_to(
						&goalie->state_machine, GS_HOLD_BALL, game_state);
				} else {
					entity_goalie_state_machine_change_state_to(
						&goalie->state_machine, GS_PROTECT_GOAL, game_state);
				}
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
			entity_ball_long_pass(ball_entity, entity_id, (v2){.y = direction}, 0.7f, game_state);
			entity_goalie_state_machine_change_state_to(
				&goalie->state_machine, GS_PROTECT_GOAL, game_state);
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
										goalie->dive_speed * 0.35f)) &&
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
				// Apply friction
				force_to_apply = Vector2Scale(Vector2Negate(Vector2Normalize(physics->velocity)),
											  50000); // TODO: this should depend on type of pitch
			} else {
				v2 steering = direction == SD_LEFT ? Vector2Left() : Vector2Right();
				force_to_apply = Vector2Scale(steering, 60000);
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
				(!dive_state_data->animation_completed ||
				 // If ball height is very low we are going to make the save even when
				 // lying on the floor after the dive
				 ball_transform->virtual_height < ball_radius)) {
				// Choose whether to bounce or hold the ball
				// Definitely hold if ball velocity is lower than 54km/h
				// Sometimes hold if ball velocity is lower than 108km/h
				// Otherwise bounce it
				dive_state_data->ball_held =
					GOALIE_HOLD_OR_BOUNCE(ball_velocity) &&
					((pitch_side == PSI_NORTH && ball_physics_3d->velocity.z < 0) ||
					 (pitch_side == PSI_SOUTH && ball_physics_3d->velocity.z > 0));

				// TODO: if the ball hits the post and comes back we don't want to hold
				// the ball
				if (dive_state_data->ball_held) {
					entity_ball_held_by_goalie(ball_entity, entity_id, game_state);
				} else {
					// Bounce ball
					v3 new_ball_velocity = Vector3Reflect(ball_physics_3d->velocity, (v3){.z = 1});
					// new_ball_velocity = Vector3RotateByAxisAngle(
					//     new_ball_velocity, (v3){.y = 1},
					//     30 - 60 * GetRandomValue(0, 100) / 100.0f * DEG2RAD);

					// Step ball position back one tick to place it outside the collider
					v3 where_to_place_ball = Vector3Add(
						V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
						Vector3Scale(Vector3Negate(ball_physics_3d->velocity), game_state->dt));

					entity_ball_set_position_and_velocity(
						ball_entity, where_to_place_ball, Vector3Scale(new_ball_velocity, 0.3f));
					entity_ball_saved_by_goalie(ball_entity, entity_id, game_state);
				}

				goalie->last_save_time = game_state->time;
				dive_state_data->ball_saved = true;
			} else if (dive_state_data->animation_completed &&
					   HAS_TIME_PASSED(
						   game_state->time, dive_state_data->animation_end_time, 0.75f)) {
				if (dive_state_data->ball_held) {
					entity_goalie_state_machine_change_state_to(
						&goalie->state_machine, GS_HOLD_BALL, game_state);
				} else {
					entity_goalie_state_machine_change_state_to(
						&goalie->state_machine, GS_PROTECT_GOAL, game_state);
				}
			}
			break;
		}
		case GS_COUNT:
			break;
		}
	}
}
