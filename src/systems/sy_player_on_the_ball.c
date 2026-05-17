#include "components.h"
#include "entities.h"
#include "e_ball.h"
#include "e_player.h"
#include "football.h"
#include "game.h"
#include "layers.h"
#include "ai.h"
#include "input.h"
#include "log.h"
#include "sprite.h"
#include "types.h"
#include <math.h>
#include <math_ext.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

struct header_phase_window {
	u8 low;
	u8 high;
	u8 optimal;
};

void systems_player_on_the_ball(struct game_state *game_state)
{
	struct entity *ball_entity = game_state->vars.ball.entity;

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
		struct state_machine_player *state_machine = &player->state_machine;

		struct component_player_controls *player_controls = &player_entity->player_controls;
		v2 player_position = player_entity->transform.position;
		v2 player_velocity = player_entity->physics.velocity;
		enum pitch_side player_pitch_side =
			game_state->vars.teams[player->team_side].play_context.pitch_side;
		f32 player_speed = Vector2Length(player_velocity);
		v2 normalized_velocity = Vector2Normalize(player_velocity);
		f32 steering_magnitude = Vector2Length(player_controls->steering);

		f32 ball_speed = Vector3Length(ball_physics_3d->velocity);

		switch (state_machine->current_state) {
		case PS_NORMAL: {
			if (ball_speed > game_state->constants.ball_max_velocity_to_gain_possession) {
				// Ball is too fast, we cannot control it
				// b32 deviate_trajectory = GetRandomValue(0, 100) % 2 == 0;
				// TODO: this deviation has to be happen just once per collision
				b32 deviate_trajectory = true;
				if (deviate_trajectory) {
					GLOG_DEBUG_CTX(LS_GAME, player_entity->header.name, "deviating trajectory\n");
					entity_ball_deviate_trajectory(
						ball_entity, player_on_the_ball_entity_id, game_state);
				}
				return;
			}

			if (player_controls->shot > 0 || player_controls->chip_shot > 0) {
				// Normal shots or chip shots are processed here

				// Shot
				player->last_kick_time = game_state->time;

				// Going over 80% of charge it's going to be bad for the shot
				// - accuracy will decrease
				// - shot power will decrease
				// - shot height will rise
				const b32 is_chip_shot = player_controls->chip_shot > 0;
				f32 power = is_chip_shot ? player_controls->chip_shot : player_controls->shot;

				f32 accuracy = game_state->vars.pitch.goal_width * 0.5f +
							   (power > 0.8f ? GetRandomValue(11, 20) : GetRandomValue(0, 10));

				v2 center_of_goal = game_state->vars.pitch.goal[1 - player_pitch_side].center;
				v2 shot_direction_to_center =
					Vector2Normalize(Vector2Subtract(center_of_goal, ball_transform->position));
				v2 shot_direction = player_controls->steering;
				if (Vector2ProjectScalar(shot_direction_to_center, shot_direction) < 0) {
					shot_direction.y *= -1;
				}
				if (game_state->constants.shoot_aid_auto_aim) {
					// Calculate shot direction for auto aim
					v2 shot_direction_to_right_post = Vector2Normalize(Vector2Subtract(
						Vector2Add(center_of_goal, (v2){.x = accuracy}), ball_transform->position));
					f32 angle_center_to_post =
						Vector2Angle(shot_direction_to_center, shot_direction_to_right_post);

					shot_direction =
						Vector2Rotate(shot_direction_to_center,
									  angle_center_to_post * player_controls->steering.x);
				}

				// Limit angle between requested shoot direction and current velocity
				f32 angle = Vector2Angle(player_velocity, shot_direction);
				f32 max_angle = 100 * DEG2RAD;
				if (angle > max_angle) {
					shot_direction = Vector2Normalize(Vector2Rotate(player_velocity, max_angle));
				} else if (angle < -max_angle) {
					shot_direction = Vector2Normalize(Vector2Rotate(player_velocity, -max_angle));
				}

				// Stick Y controls shot height: toward goal = high, away = low
				f32 attack_sign = shot_direction_to_center.y > 0 ? -1.0f : 1.0f;
				f32 toward_goal = player_controls->steering.y * attack_sign;
				// 0.4 should allow a shot to hit the goal even when the stick if fully tilted towards goal
				// unless shot power is overcharged
				f32 shot_height = Clamp01((1.0f - toward_goal) * 0.4f);

				// Calculate shot force
				// Max power is achieved at 80%, if you go over you actually lose power
				if (power > 0.8f) {
					f32 overcharge_pct = (power - 0.8f) / 0.2f;
					// Lose 20% of power immediately and another 12,5% proportionally to overcharge
					power = 0.8f - 0.1f * overcharge_pct;
					// Shot height is increased by 30% immediately and by another 30% proportionally to overcharge
					shot_height *= 1 + 0.3f + 0.3f * overcharge_pct;
				} else {
					power = power / 0.8f;
				}

				entity_ball_shoot(ball_entity,
								  player_on_the_ball_entity_id,
								  shot_direction,
								  shot_height,
								  power,
								  is_chip_shot,
								  game_state);
				kicked_ball = true;
			} else if (player_controls->pass > 0) {
				v2 steering = player_controls->steering;
				if (steering_magnitude < 0.1f) {
					steering = ORIENTATION_VECTORS[player_entity->sprite.orientation];
				}

				// Pass
				player->last_kick_time = game_state->time;
				v3 pass_direction;
				struct entity *teammate_entity;
				b32 found = entity_player_find_pass_target(
					player_entity, steering, 45, 300, &teammate_entity, game_state);
				f32 power = player_controls->pass;
				b32 automatic_pass = found && player_controls->automatic_pass;
				if (automatic_pass) {
					struct component_transform *teammate_transform = &teammate_entity->transform;
					v2 to_teammate = Vector2Subtract(teammate_transform->position, player_position);
					pass_direction = Vector3Normalize(
						V2_TO_V3(to_teammate, game_state->constants.ball_pass_height));
					ball->pass_target_entity_id = teammate_entity->header.id;
					power = Clamp01(Vector2Length(to_teammate) / 300.0f);
				} else {
					pass_direction = Vector3Normalize(
						V2_TO_V3(steering, game_state->constants.ball_pass_height));
				}
				entity_ball_short_pass(ball_entity,
									   player_on_the_ball_entity_id,
									   pass_direction,
									   power,
									   automatic_pass,
									   game_state);
				kicked_ball = true;
			} else if (player_controls->long_pass > 0 || player_controls->long_driven_pass > 0) {
				v2 steering = player_controls->steering;
				if (steering_magnitude < 0.1f) {
					steering = ORIENTATION_VECTORS[player_entity->sprite.orientation];
				}

				v2 normalised_steering = Vector2Normalize(steering);
				v2 long_pass_direction = normalised_steering;
				const b32 is_driven = player_controls->long_driven_pass > 0;
				const f32 power = is_driven ? player_controls->long_driven_pass :
											  player_controls->long_pass;

				player->last_kick_time = game_state->time;
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
									  power,
									  is_driven,
									  game_state);
				kicked_ball = true;
			} else if (!kicked_ball) {
				// For ball control we use velocity directly because it's easier and produce
				// better results
				if (player_speed < game_state->constants.player_run_max_velocity * 0.8f) {
					player->control_ball = true;
					player->time_spent_in_run_control = 0;
				}

				// Body collision inhibits ball control.
				// Duration scales with impulse: light brush ~ 0.05s, heavy charge ~ 0.25s.
				f32 inhibit_duration =
					Clamp(player->last_body_collision_impulse * 0.0005f, 0.05f, 0.25f);
				if (!HAS_TIME_PASSED(
						game_state->time, player->last_body_collision_time, inhibit_duration)) {
					break;
				}

				if (player->control_ball) {
					// Precision control
					player->time_spent_in_precision_control += game_state->dt;
					player->time_spent_in_run_control = 0;

					// If the player is not moving we reset time spent in precision control
					// TODO: we need to verify if this gives an unfair advantage
					if (steering_magnitude < 0.01f) {
						player->time_spent_in_precision_control = 0;
					}

					// Stop button keeps the player in precision control
					if (player_controls->stop) {
						player->time_spent_in_precision_control = 0;
					}

					// Every 250ms player does a longer control if he's running at > 70% top speed
					if (player->time_spent_in_precision_control > 0.25f &&
						player_speed > game_state->constants.player_run_max_velocity * 0.7f) {
						player->control_ball = false;
					}
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

					// If player is not moving we position the ball on the collider radius with the same
					// angle that exists between player and ball.
					if (player_speed < 0.01f) {
						position_to_seek = Vector2Add(
							player_position,
							Vector2Scale(Vector2Normalize(Vector2Subtract(ball_transform->position,
																		  player_position)),
										 player_entity->collider.colliders[0].props.circle.radius));
					}

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
				game_controller_player_reset(player_on_the_ball_entity_id, game_state);
			}
			break;
		}
		case PS_SLIDING_TACKLE: {
			struct component_physics *player_physics = &player_entity->physics;
			f32 tackle_force =
				Vector2Length(Vector2Scale(player_physics->velocity, player->collision_force)) /
				player_physics->max_velocity;

			GLOGF_DEBUG_CTX(
				LS_GAME, player_entity->header.name, "tackle force: %.2f\n", tackle_force);
			entity_ball_tackle(ball_entity,
							   player_on_the_ball_entity_id,
							   V2_TO_V3(player->collision_direction, 0),
							   tackle_force,
							   game_state);

			break;
		}
		case PS_HEADER: {
			static struct header_phase_window header_phase_windows[] = {
				{17, 22, 20}, {21, 26, 23}, {24, 29, 26}, {26, 31, 28}};

			if (player_entity->player.state_machine.state_data.header.phase == HP_GROUND) {
				// Cannot hit if we are preparing the jump
				break;
			}

			struct component_physics *player_physics = &player_entity->physics;

			v2 hit_direction;

			enum header_phase header_phase =
				player_entity->player.state_machine.state_data.header.phase;
			f32 ball_height = ball_transform->virtual_height;

			// We normalize later
			if (Vector2Length(player_controls->steering) > 0.1f) {
				hit_direction = player_controls->steering;
			} else {
				// If steering is zero we use the orientation of player's sprite
				hit_direction = ORIENTATION_VECTORS[player_entity->sprite.orientation];
			}

			// Here we determine how hit direction is compatible with ball direction.
			// Logic is if the ball is moving in the same direction of the hit we will not be
			// able to apply any force at all.
			f32 hit_direction_impact = 1;
			// Projection is positive if vectors are mostly aligned, negative is they are opposite
			f32 hit_direction_projection_on_ball_velocity = Vector2ProjectScalar(
				hit_direction, Vector2Normalize(V3_TO_V2(ball_physics_3d->velocity)));
			if (hit_direction_projection_on_ball_velocity > 0.5f) {
				// After 90 degrees of angle between hit and ball direction the hit is considered suboptimal
				// because it becomes difficult to apply force
				hit_direction_impact = 1 - (hit_direction_projection_on_ball_velocity - 0.5f) * 2;
			}

			// If player is in the falling phase of the header we won't be able to apply any force.
			f32 fall_down_impact = 1;
			if (player_entity->player.state_machine.state_data.header.should_fall_down) {
				fall_down_impact =
					1 -
					(player_entity->player.state_machine.state_data.header.max_elevation_frame_id -
					 player_entity->player.state_machine.state_data.header.last_frame_id) /
						10.0f;
			}

			f32 ball_height_impact = 0.0f;
			f32 impact_vertical_angle = 0.0f;

			f32 header_force_mult = 0;

			const f32 ball_radius = ball_collider->colliders[0].props.circle.radius;

			b32 fumble = false;

			// There are some conditions that will make the header look like shit
			if (ball_height > game_state->constants.player_header_min_height) {
				// Ball is high enough to be hit with an header

				struct header_phase_window hpw = header_phase_windows[header_phase - 1];

				// We get the difference between max and min hittable ball height.
				// This is the total height valid for a header, then we divide it by 4 header phases and we get how
				// big is each phase height.
				// At this point we can determine how well we hit the ball based on header phase and height of the ball.
				// This will also affect the direction of the ball.
				const f32 header_phase_size = hpw.high - hpw.low;

				if (ball_height > (hpw.high + ball_radius)) {
					// Ball is too high, no hit!
					GLOGF_DEBUG_CTX(LS_GAME,
									player_entity->header.name,
									"missed header, ball too high: %.2f > %d\n",
									ball_height,
									hpw.high);
					break;
				}

				const f32 delta_from_optimal_height = ball_height - hpw.optimal;
				const f32 impact_factor =
					Clamp(delta_from_optimal_height / header_phase_size, -1, 1);
				// We clamp the impact_vertical_angle factor (-0.5, 0.5) because we want to avoid very angled hits
				// We also raise it to the power of two, so that it's more forgiving
				impact_vertical_angle = impact_factor * impact_factor * 0.5f;

				ball_height_impact = 1 - fabsf(impact_factor);

				GLOGF_DEBUG_CTX(
					LS_GAME,
					player_entity->header.name,
					"ball height: %.2f - HP: %d - ball_height: %.2f - hit_direction: %.2f - collision: %.2f - fall_down: %.2f - vertical_angle: %.2f\n",
					ball_height,
					header_phase,
					ball_height_impact,
					hit_direction_impact,
					player->collision_force,
					fall_down_impact,
					impact_vertical_angle);

				// Weight of different impacts:
				// - steering -> 1
				// - collision force -> 1
				// - fall down -> 1
				// - ball height -> 1
				header_force_mult = (hit_direction_impact + player->collision_force +
									 fall_down_impact + ball_height_impact) /
									4.0f;

				// Passes work just like shoots but 75% of the force.
				if (player->state_machine.state_data.header.type == HEADER_TYPE_PASS)
					header_force_mult *= 0.75f;

				if (ball_height < hpw.low) {
					// Ball is too low, just fumble!
					GLOGF_DEBUG_CTX(LS_GAME,
									player_entity->header.name,
									"fumbled header, ball too low for header phase: %.2f < %d\n",
									ball_height,
									hpw.low);
					fumble = true;
				}
			} else {
				GLOGF_DEBUG_CTX(LS_GAME,
								player_entity->header.name,
								"fumbled header, ball is not high enough to be hit: %d\n",
								header_phase);
				fumble = true;
			}

			f32 header_force;
			if (fumble) {
				header_force = game_state->constants.ball_header_fumble_force;
			} else {
				// We take the max velocity between player and ball to determine the force we should apply to the ball
				//
				header_force =
					MAX(header_force_mult * sqrt(MAX(Vector2LengthSqr(player_physics->velocity),
													 Vector3LengthSqr(ball_physics_3d->velocity))),
						game_state->constants.ball_header_min_force);
			}

			GLOGF_DEBUG_CTX(LS_GAME,
							player_entity->header.name,
							"header [mult: %.2f, force:%.2f]\n",
							header_force_mult,
							header_force);

			entity_ball_header(ball_entity,
							   player_on_the_ball_entity_id,
							   Vector3Normalize(V2_TO_V3(hit_direction, impact_vertical_angle)),
							   header_force,
							   game_state);

			entity_player_header_hit(player_entity, game_state);

			break;
		}
		case PS_GOALIE:
		case PS_THROW_IN:
		case PS_FALL:
		case PS_NONE:
		case PS_COUNT:
			break;
		}
	}
}
