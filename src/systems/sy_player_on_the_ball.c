#include "components.h"
#include "entities.h"
#include "e_ball.h"
#include "e_player.h"
#include "game.h"
#include "layers.h"
#include <math.h>
#include <math_ext.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include "ai.h"
#include "types.h"

void systems_player_on_the_ball(struct game_state *game_state)
{
	struct entity *ball_entity = game_state->vars.ball_entity;

	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
	UNUSED(ball_physics_3d);
	UNUSED(ball_collider);

	// Player can control the ball only if he's colliding with it in this frame
	const entity_id_t player_on_the_ball_entity_id =
		entity_ball_player_on_the_ball(ball_entity, game_state);

	b32 kicked_ball = false;
	if (player_on_the_ball_entity_id != EID_NOT_VALID) {

		struct entity *player_entity =
			entity_get(game_state->entity_repository, player_on_the_ball_entity_id);

		struct component_player *player = &player_entity->player;
		struct state_machine_player *state_machine = player->state_machine;

		switch (state_machine->current_state) {
		case PS_NORMAL: {
			struct component_player_controls *player_controls = &player_entity->player_controls;
			v2 player_position = player_entity->transform.position;
			v2 player_velocity = player_entity->physics.velocity;
			f32 player_speed = Vector2Length(player_velocity);
			v2 normalized_velocity = Vector2Normalize(player_velocity);
			f32 steering_magnitude = Vector2Length(player_controls->steering);

			f32 ball_speed = Vector3Length(ball_physics_3d->velocity);
			if (ball_speed > game_state->constants.ball_max_velocity_to_gain_possession) {
				// Ball is too fast, we cannot control it
				// b32 deviate_trajectory = GetRandomValue(0, 100) % 2 == 0;
				// TODO: this deviation has to be happen just once per collision
				b32 deviate_trajectory = true;
				if (deviate_trajectory) {
					printf("deviating trajectory\n");
					entity_ball_deviate_trajectory(
						ball_entity, player_on_the_ball_entity_id, game_state);
				}
				return;
			}

			if (player_controls->shot > 0) {
				// Shot
				player->last_kick_time = game_state->time;

				// v2 center_of_goal = game_state->vars.pitch.goal[1 - player_pitch_side].center;
				// v2 shot_direction_to_center =
				// 	Vector2Normalize(Vector2Subtract(center_of_goal, ball_transform->position));
				// v2 shot_direction_to_right_post = Vector2Normalize(Vector2Subtract(
				// 	Vector2Add(center_of_goal, (v2){.x = accuracy}), ball_transform->position));
				// f32 angle_center_to_post =
				// 	Vector2Angle(shot_direction_to_center, shot_direction_to_right_post);

				// v2 shot_direction = Vector2Rotate(
				// 	shot_direction_to_center, angle_center_to_post * player_controls->steering.x);
				v2 shot_direction = player_controls->steering;
				f32 shot_height = player_controls->shot;

				// Limit angle between requested shoot direction and current velocity
				f32 angle = Vector2Angle(player_velocity, shot_direction);
				f32 max_angle = 100 * DEG2RAD;
				if (angle > max_angle) {
					shot_direction = Vector2Normalize(Vector2Rotate(player_velocity, max_angle));
				} else if (angle < -max_angle) {
					shot_direction = Vector2Normalize(Vector2Rotate(player_velocity, -max_angle));
				}
				entity_ball_shoot(ball_entity,
								  player_on_the_ball_entity_id,
								  shot_direction,
								  shot_height,
								  player_controls->shot,
								  game_state);
				kicked_ball = true;
			} else if (player_controls->pass > 0 && steering_magnitude > 0.1f) {
				// Pass
				player->last_kick_time = game_state->time;
				v3 pass_direction;
				struct entity *teammate_entity =
					entity_player_find_pass_target(player_entity, 35, 300, game_state);
				f32 power = player_controls->pass;
				b32 automatic_pass = teammate_entity != NULL && player_controls->automatic_pass;
				if (automatic_pass) {
					struct component_transform *teammate_transform = &teammate_entity->transform;
					v2 to_teammate = Vector2Subtract(teammate_transform->position, player_position);
					pass_direction = Vector3Normalize(V2_TO_V3(to_teammate, 0));
					ball->pass_target_entity_id = teammate_entity->header.id;
					power = Vector2Length(to_teammate) / 300.0f;
				} else {
					pass_direction = Vector3Normalize(V2_TO_V3(player_controls->steering, 0));
				}
				entity_ball_short_pass(ball_entity,
									   player_on_the_ball_entity_id,
									   pass_direction,
									   power,
									   automatic_pass,
									   game_state);
				kicked_ball = true;
			} else if (player_controls->long_pass && steering_magnitude > 0.1f) {
				// Long pass
				player->last_kick_time = game_state->time;
				v2 long_pass_direction = Vector2Normalize(player_controls->steering);
				// Limit angle between requested shoot direction and current velocity
				f32 angle = Vector2Angle(player_velocity, long_pass_direction);
				f32 max_angle = 120 * DEG2RAD;
				if (angle > max_angle) {
					long_pass_direction =
						Vector2Normalize(Vector2Rotate(player_velocity, max_angle));
				} else if (angle < -max_angle) {
					long_pass_direction =
						Vector2Normalize(Vector2Rotate(player_velocity, -max_angle));
				}
				entity_ball_long_pass(ball_entity,
									  player_on_the_ball_entity_id,
									  long_pass_direction,
									  player_controls->long_pass,
									  game_state);
				kicked_ball = true;
			} else if (!kicked_ball && steering_magnitude > 0.01f) {
				// For ball control we use velocity directly because it's easier and produce
				// better results
				if (player_speed < game_state->constants.player_run_max_velocity * 0.8f) {
					player->control_ball = true;
					player->time_spent_in_run_control = 0;
				}
				if (player->control_ball) {
					// Precision control
					player->time_spent_in_precision_control += game_state->dt;
					player->time_spent_in_run_control = 0;
					// Every 250ms player does a longer control if he's running at > 70% top speed
					if (player->time_spent_in_precision_control > 0.25f &&
						player_speed > game_state->constants.player_run_max_velocity * 0.7f) {
						player->control_ball = false;
					}
					layer_debug_queue_draw_text(
						game_state->layer_debug,
						TextFormat("prec_ctrl: %.2f", player->time_spent_in_precision_control));
					/*
				 * Control difficulty is affected by multiple variables:
				 * - time on the ball: more time on the ball, control gets more difficult
				 * - speed: higher speed, harder to control the ball
				 * - ball height: higher the ball, harder to control
				 * - pitch conditions (TODO)
				 *
				 * Control difficulty determines the force that will applied to the ball, which
				 * will be higher if difficulty is lower.
				 * If difficulty is higher than what the player's quality allows, they will likely
				 * lose the ball.
				 */
					f32 precision_control_max_seconds = 1.0f;
					f32 time_on_ball_mult = Clamp01(player->time_spent_in_precision_control /
													precision_control_max_seconds);

					// The faster the player is running the less control
					f32 speed_mult =
						Clamp01(powf(player_speed, 4) /
								powf(game_state->constants.player_control_max_velocity, 4));

					// The higher the ball is the less control the player has
					f32 ball_height = ball_transform->virtual_height;
					f32 ball_height_mult =
						(ball_height - 2.5f) / (game_state->constants.player_control_height -
												2.5f); // Ball center is 2.5f over pitch

					// Let's give weight to each multiplier
					f32 control_difficulty =
						(speed_mult * 1 + ball_height_mult * 1 + time_on_ball_mult * 2) / 4.0f;
					// printf("cq: %.2f sm: %.2f bm: %.2f tm: %.2f\n",
					//        control_difficulty,
					//        speed_mult,
					//        ball_height_mult,
					//        time_on_ball_mult);

					v2 position_to_seek = Vector2Add(
						player_position,
						Vector2Scale(normalized_velocity,
									 player_entity->collider.colliders[0].props.circle.radius));

					// This threshold should depend on player quality.
					// If difficulty is higher than the threshold the player will likely lose the ball.
					if (control_difficulty < 0.7f)
						entity_ball_control_precision(ball_entity,
													  player_velocity,
													  position_to_seek,
													  control_difficulty,
													  game_state);
				} else {
					// Run control
					player->time_spent_in_run_control += game_state->dt;
					player->time_spent_in_precision_control = 0;
					if (player->time_spent_in_run_control > 0.25f) {
						player->control_ball = true;
					}
					v2 control_direction = player_controls->steering;

					// The control ratio should depend on various variables:
					// - speed of the player
					// - technical abilities of the player
					// - height of the ball
					// - pitch conditions (TODO)
					//
					// Each one should have a specific weight applied.
					//
					// Range is 0.0-1.0 with 1.0 being the best control quality
					//
					// This will take some time to get right
					//
					// TODO: add what's missing

					// Player quality
					f32 player_quality_mult = 1 - 1.0f;

					// The faster the player is running the less control
					f32 speed_mult =
						Clamp01(powf(player_speed, 4) /
								powf(game_state->constants.player_run_max_velocity, 4));
					// The higher the ball is the less control the player has
					f32 ball_height = ball_transform->virtual_height;
					f32 ball_height_mult =
						(ball_height - 2.5f) / (game_state->constants.player_control_height -
												2.5f); // Ball center is 2.5f over pitch

					// Let's give weight to each multiplier
					f32 control_difficulty =
						(speed_mult * 1 + ball_height_mult * 1 + player_quality_mult * 1) / 3.0f;

					entity_ball_control_running(ball_entity,
												control_direction,
												player_velocity,
												control_difficulty,
												game_state);
				}
			}

			player_entity->player.kicked_ball = kicked_ball;

			if (kicked_ball) {
				game_input_player_charge_reset(game_state, player_on_the_ball_entity_id);
			}
			break;
		}
		case PS_SLIDING_TACKLE: {
			struct component_physics *player_physics = &player_entity->physics;
			f32 tackle_force =
				Vector2Length(Vector2Scale(player_physics->velocity, player->collision_force)) /
				player_physics->max_velocity;

			printf("tackle force: %.2f\n", tackle_force);
			entity_ball_tackle(ball_entity,
							   player_on_the_ball_entity_id,
							   V2_TO_V3(player->collision_direction, 0),
							   tackle_force,
							   game_state);

			break;
		}
		case PS_GOALIE:
		case PS_THROW_IN:
		case PS_HEADER:
		case PS_FALL:
		case PS_NONE:
		case PS_COUNT:
			break;
		}
	}
}
