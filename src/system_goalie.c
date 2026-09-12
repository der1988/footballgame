#include "system.h"
#include "entity_components.h"
#include "entity_ball.h"
#include "entity_player.h"
#include "entity.h"
#include "football.h"
#include "game.h"
#include "input.h"
#include "macro.h"
#include "math_ext.h"
#include "constants.h"
#include "physics.h"
#include "sprite.h"
#include "types.h"
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include "collision.h"

// Height the ball leaves his hands at when he rolls it out
#define GOALIE_THROW_HEIGHT 6
// How long he holds the follow-through pose once the ball has gone
#define GOALIE_THROW_FOLLOW_THROUGH_TIME 0.5f
// He carries the ball slower than he runs without it
#define GOALIE_CARRY_SPEED_RATIO 0.75f
// And pulls up harder than a player running free, so the ball in his hands has some weight
#define GOALIE_CARRY_DRAG_RATIO 1.5f
// Height the ball sits at while it is in his hands
#define GOALIE_HOLD_BALL_HEIGHT 12

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

		entity_player_goalie_update_role(entity, game_state);

		if (player->state_machine.current_state != PS_GOALIE) {
			// Out of goal he is a field player, but a ball that comes into his area is still his to
			// catch. The field player's ball test only reaches control height, which a header or a
			// cross flies straight over, so the goalie's reach is used here.
			const struct collision_information ball_within_reach_info =
				entity_player_is_colliding_with_ball(
					entity,
					PCT_GOALIE,
					V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
					ball_entity->physics3d.velocity,
					ball_collider->colliders[0].props.circle.radius,
					game_state->constants.crossbar_height);
			if (ball_within_reach_info.is_colliding) {
				// Refuses by itself when the laws or his position say no
				entity_player_goalie_pick_up_ball(entity, game_state);
			}
			continue;
		}
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

		// A ball a team mate played back to him has to be played with his feet
		const b32 may_handle_ball = entity_ball_may_goalie_handle(ball_entity, entity, game_state);

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
				if (!may_handle_ball) {
					// No hands on this one, so he comes out of goal and plays it as a field player
					state_machine_player_fire(&player->state_machine, PT_STEP_OUT, game_state);
					break;
				}

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

			const f32 exit_max_speed = player->constants.run_max_velocity;

			struct future_position *future_reachable_position =
				&ball->future_positions[FUTURE_POSITION_REACHABLE_TO_GOALIE];

			// can we get the ball in our hands without moving by any chance?
			if (may_handle_ball &&
				ball_transform->virtual_height >
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
					force = player->constants.movement_force;

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
						force = player->constants.movement_force;

						destination = V3_TO_V2(prediction.future_pos);
						steering = entity_player_steer_arrive(entity, destination, game_state);
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
						physics->max_velocity = player->constants.run_max_velocity * 0.5f;
						force = player->constants.movement_force;
						destination = intersection_point;
						move_on_the_line = true;
						break;
					}
				}

				if (!move_on_the_line) {
					physics->max_velocity = player->constants.run_max_velocity;
					force = player->constants.movement_force;
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

			if (may_handle_ball &&
				ball_velocity > game_state->constants.goalie_dive_min_ball_velocity &&
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
				jump_state_data->ball_held = may_handle_ball && GOALIE_HOLD_OR_BOUNCE(ball_velocity);

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
			const i8 direction = pitch_side == PSI_NORTH ? 1 : -1;
			const v2 new_ball_position = Vector2Add(
				transform->position,
				Vector2Scale((v2){.y = direction},
							 goalie_collider->props.circle.offset.y +
								 goalie_collider->props.circle.radius + ball_radius + 7));

			// He can run with it in his hands, slower than he runs without it
			physics->max_velocity = player->constants.run_max_velocity * GOALIE_CARRY_SPEED_RATIO;

			const v2 carry_steering = entity->player_controls.steering;
			v2 carry_force = Vector2Scale(carry_steering, player->constants.movement_force);

			// Let go of the stick and he pulls up instead of gliding on
			if (Vector2Length(carry_steering) < 0.01f &&
				!physics_set_at_rest_if_needed(physics, PLAYER_AT_REST_SPEED)) {
				carry_force = Vector2Add(carry_force,
										 physics_drag_force(physics->velocity,
															player->constants.drag *
																GOALIE_CARRY_DRAG_RATIO));
			}

			entity_player_physics_apply_force(entity, carry_force, game_state);
			entity_player_physics_apply_velocity(entity, game_state);

			// Carrying it out of his area is not allowed, so he is stopped at the edge
			const rect_t penalty_box = game_state->vars.pitch.goal[pitch_side].penalty_box;
			const v2 position_in_box = {
				.x = Clamp(transform->position.x, penalty_box.x, penalty_box.x + penalty_box.width),
				.y = Clamp(
					transform->position.y, penalty_box.y, penalty_box.y + penalty_box.height)};
			if (position_in_box.x != transform->position.x) {
				physics->velocity.x = 0;
			}
			if (position_in_box.y != transform->position.y) {
				physics->velocity.y = 0;
			}
			transform->position = position_in_box;

			// His own frames draw the ball, so the entity is only kept with his hands to keep the
			// camera, the lookaheads and the out of bounds checks honest.
			entity_ball_set_position_and_velocity(
				ball_entity,
				V2_TO_V3(Vector2Add(transform->position,
									Vector2Scale((v2){.y = direction},
												 goalie_collider->props.circle.radius)),
						 GOALIE_HOLD_BALL_HEIGHT),
				Vector3Zero());
			// Holding on to it is possession, however long he holds it for
			entity_ball_set_last_touch(ball, entity_id, game_state);

			// Winding up a throw he holds the ball out ready, facing where it is going
			if (entity->player_controls.charging_throw) {
				if (!IS_V2_ZERO(entity->player_controls.steering)) {
					sprite->orientation =
						sprite_calculate_orientation(entity->player_controls.steering);
				}
				struct asset_animation *throw_animation = asset_animation_get_by_id(
					game_state->assets.animation_cache,
					game_state->gfx.goalie.hand_pass_animations[sprite->orientation]);
				sprite->frame_id = throw_animation->frames[0];
			} else {
				// Standing still he holds it at his chest, which the art only has facing up or down
				asset_sprite_frame_id_t hold_frames[SO_COUNT];
				for (usize i = 0; i < SO_COUNT; ++i) {
					hold_frames[i] = game_state->gfx.goalie.save_frames[pitch_side][ST_WITH_BALL];
				}
				entity_player_anim_calculate_running_frame(
					entity,
					hold_frames,
					game_state->gfx.goalie.run_ball_animations,
					physics->velocity,
					5,
					game_state);
			}

			if (entity->player_controls.put_down) {
				entity_ball_back_in_play(ball_entity, game_state);
				entity_ball_set_position_and_velocity(
					ball_entity, V2_TO_V3(new_ball_position, ball_radius), Vector3Zero());
				state_machine_goalie_fire(&goalie->state_machine, GT_RELEASE, game_state);
				state_machine_player_fire(&player->state_machine, PT_STEP_OUT, game_state);
				input_reset_player(entity_id, game_state);
				break;
			}

			if (entity->player_controls.pass > 0) {
				// Thrown out by hand. The team mate is picked the way an automatic pass picks one.
				v2 throw_direction = entity->player_controls.steering;
				if (IS_V2_ZERO(throw_direction)) {
					throw_direction = (v2){.y = direction};
				}

				const f32 max_distance = game_state->constants.ball_pass_auto_max_distance;
				struct entity *target_entity;
				if (entity_player_find_pass_target(entity,
												   throw_direction,
												   30,
												   entity->player_controls.pass * max_distance,
												   max_distance,
												   &target_entity,
												   game_state)) {
					throw_direction =
						Vector2Subtract(target_entity->transform.position, transform->position);
					ball->pass_target_entity_id = target_entity->header.id;
				}

				goalie->throw_direction = Vector2Normalize(throw_direction);
				goalie->throw_power = entity->player_controls.pass;
				sprite->orientation = sprite_calculate_orientation(goalie->throw_direction);

				const v2 ball_release_position =
					Vector2Add(transform->position,
							   Vector2Scale(goalie->throw_direction,
											goalie_collider->props.circle.radius + ball_radius + 2));
				entity_ball_back_in_play(ball_entity, game_state);
				entity_ball_set_position_and_velocity(
					ball_entity, V2_TO_V3(ball_release_position, GOALIE_THROW_HEIGHT), Vector3Zero());
				// Rolled out along the ground, so no height on the direction
				entity_ball_short_pass(ball_entity,
									   entity_id,
									   V2_TO_V3(goalie->throw_direction, 0),
									   goalie->throw_power,
									   true,
									   game_state);

				state_machine_goalie_fire(&goalie->state_machine, GT_THROW, game_state);
				break;
			}

			if (entity->player_controls.long_pass > 0) {
				// Kicked long out of his hands, the same kick the AI makes below
				entity_ball_back_in_play(ball_entity, game_state);
				entity_ball_set_position_and_velocity(
					ball_entity, V2_TO_V3(new_ball_position, 10), Vector3Zero());
				entity_ball_long_pass(ball_entity,
									  entity_id,
									  (v2){.y = direction},
									  entity->player_controls.long_pass,
									  false,
									  game_state);
				state_machine_goalie_fire(&goalie->state_machine, GT_RELEASE, game_state);
				break;
			}

			// Human or AI, he leaves GS_HOLD_BALL only through the controls above
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
				dive_state_data->ball_held = may_handle_ball &&
											 GOALIE_HOLD_OR_BOUNCE(ball_velocity) &&
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
		case GS_THROW: {
			// The ball has already gone: this is the follow-through, held long enough to read.
			struct asset_animation *animation = asset_animation_get_by_id(
				game_state->assets.animation_cache,
				game_state->gfx.goalie.hand_pass_animations[sprite->orientation]);
			sprite->frame_id = animation->frames[animation->frames_count - 1];

			if (HAS_TIME_PASSED(game_state->time,
								goalie->state_machine.state_data.throw_ball.start_time,
								GOALIE_THROW_FOLLOW_THROUGH_TIME)) {
				state_machine_goalie_fire(&goalie->state_machine, GT_RELEASE, game_state);
			}
			break;
		}
		case GS_COUNT:
			break;
		}
	}
}
