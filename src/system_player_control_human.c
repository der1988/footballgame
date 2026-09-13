#include "system.h"
#include "entity_components.h"
#include "entity.h"
#include "entity_ball.h"
#include "entity_player.h"
#include "football.h"
#include "game.h"
#include "input.h"
#include "macro.h"
#include "constants.h"
#include "math_ext.h"
#include "sprite.h"
#include <raymath.h>
#include <string.h>

void system_player_control_human(struct game_state *game_state)
{
	struct entity *ball_entity = game_state->vars.ball.entity;

	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
	UNUSED(ball_physics_3d);
	UNUSED(ball_collider);
	UNUSED(ball_transform);
	UNUSED(ball);

	for (usize human_player_idx = 0; human_player_idx < MAX_HUMAN_PLAYERS; ++human_player_idx) {
		if (game_state->human_players[human_player_idx].controlled_player.entity_id ==
			EID_NOT_VALID) {
			continue;
		}

		struct human_player *human_player = &game_state->human_players[human_player_idx];
		if (!human_player || !human_player->enabled) {
			continue;
		}

		struct entity *entity =
			entity_get(game_state->entity_repository, human_player->controlled_player.entity_id);

		if (!entity_is_alive_and_has_components(
				entity, 5, C_TRANSFORM, C_PHYSICS, C_SPRITE, C_PLAYER, C_COLLIDER)) {
			continue;
		}

		DECLARE_ENTITY_COMPONENT(entity, player);

		struct state_machine_player *state_machine = &player->state_machine;

		struct component_player_controls *player_controls = &entity->player_controls;
		RESET_COMPONENT(player_controls);
		player_controls->pass_target_entity_id = EID_NOT_VALID;

		struct entity *last_entity_that_touched_ball =
			entity_get(game_state->entity_repository, ball->last_touch_entity_id);

		// Player is human controlled
		struct game_controller *game_controller =
			&game_state->input_manager->game_controllers[human_player->game_controller_id];
		struct game_controller_state *game_controller_state = &game_controller->state;

		player_controls->steering = input_movement_vector(game_controller);

		switch (state_machine->current_state) {
		case PS_NONE:
			break;
		case PS_NORMAL: {
			// Off the ball
			if (!entity->player.is_colliding_with_ball) {
				const b32 action_button_pressed =
					game_controller_state->current[GAME_CONTROL_SHOOT].digital.pressed ||
					game_controller_state->current[GAME_CONTROL_AUTOMATIC_PASS].digital.pressed ||
					game_controller_state->current[GAME_CONTROL_PASS].digital.pressed;

				// Header
				b32 header_started = false;
				if (action_button_pressed) {
					if (entity_player_should_jump_for_header(entity, game_state)) {
						player_controls->header_shot =
							game_controller_state->current[GAME_CONTROL_SHOOT].digital.pressed;
						player_controls->header_pass = !player_controls->header_shot;
						header_started = true;
					}
				}

				// Sliding tackle
				if (!header_started &&
					game_controller_state->current[GAME_CONTROL_SLIDING_TACKLE].digital.pressed) {
					if (last_entity_that_touched_ball &&
						last_entity_that_touched_ball->header.id != entity->header.id &&
						(last_entity_that_touched_ball->player.team_side !=
							 entity->player.team_side ||
						 football_pitch_is_position_inside_penalty_area(
							 entity->transform.position,
							 game_state->vars.teams[entity->player.team_side]
								 .play_context.pitch_side,
							 game_state))) {
						// We don't want to trigger a tackle if:
						// - we are running and we just want to make a long pass
						// - we are running to get a pass from our teammate and we want to cross immediately
						// - our gk just rebounded a shot and we want to do a sliding tackle to clear the ball
						player_controls->sliding_tackle = true;
					}
				}
				// Tackle. It shares a button with the automatic pass, so it may only stand in
				// for it while the opponents have the ball: that is exactly when the charge
				// lock refuses to charge, so a press is never both a charge and a tackle.
				// Held, not pressed: the button runs the player at the ball for as long as he
				// holds it, and the challenge goes in on its own once he reaches it.
				else if (!header_started &&
						 game_controller_state->current[GAME_CONTROL_TACKLE].digital.down &&
						 entity_ball_team_in_possession(ball_entity, game_state) !=
							 entity->player.team_side) {
					player_controls->steering = entity_player_steer_pursuit(
						entity, ball_transform->position, V3_TO_V2(ball_physics_3d->velocity));
					player_controls->steering_assisted = true;
				} else if (!action_button_pressed && IS_V2_ZERO(player_controls->steering)) {
					// This section only triggers if the player is not giving meaningful directional input
					if (ball->pass_target_entity_id == entity->header.id) {
						// the target of an automatic pass runs to meet the ball on his own, so that
						// selecting him doesn't force the human to steer him there by hand. the stick
						// takes the run over as soon as it leaves the dead zone and gives it back as soon
						// as it recenters; buttons keep working throughout.
						player_controls->steering =
							entity_player_steer_arrive(entity,
													   entity_player_position_to_seek_to_reach_ball(
														   entity, ball_entity, game_state),
													   game_state);
						player_controls->steering_assisted = true;
					} else if (ball_transform->virtual_height > HEADER_MAX_HEIGHT &&
							   game_state->vars.ball.entity->ball
								   .future_positions[FUTURE_POSITION_BOUNCE]
								   .available) {
						// let's steer the player towards where the ball should be reachable again:
						// this is going to help a lot with headers.
						player_controls->steering = entity_player_steer_arrive(
							entity,
							V3_TO_V2(game_state->vars.ball.entity->ball
										 .future_positions[FUTURE_POSITION_BOUNCE]
										 .position),
							game_state);
						player_controls->steering_assisted = true;
					}
				}
			}
			// On the ball
			else if (entity->player.is_colliding_with_ball) {
				// Tackle. The ball is in reach but the opponents still hold it, so the button
				// goes in to win it rather than playing it. Winning it flips possession, which
				// is what stops the challenge repeating.
				if (game_controller_state->current[GAME_CONTROL_TACKLE].digital.down &&
					entity_ball_team_in_possession(ball_entity, game_state) !=
						entity->player.team_side) {
					player_controls->tackle = true;
				}
				// Stop
				if (game_controller_state->current[GAME_CONTROL_STOP].digital.down) {
					player_controls->stop = true;
				}
				// Kick
				// First we check if the charge lock is charged
				if (game_controller->charge_lock.state == INPUT_CHARGE_LOCK_CHARGED) {
					fg_time_t time_holding_button = game_controller->charge_lock.end_time -
													game_controller->charge_lock.start_time;
					switch (game_controller->charge_lock.control_id) {
					case GAME_CONTROL_SHOOT: {
						f32 power = SHOOT_POWER(time_holding_button);
						if (game_controller_state->current[GAME_CONTROL_MODIFIER_LEFT].digital.down) {
							player_controls->chip_shot = power;
						} else {
							player_controls->shot = power;
						}
						break;
					}
					case GAME_CONTROL_AUTOMATIC_PASS: {
						f32 power = PASS_POWER(time_holding_button);
						player_controls->pass = power;
						player_controls->automatic_pass = true;
						player_controls->pass_target_entity_id = entity_player_choose_pass_target(
							entity,
							player_controls->steering,
							ORIENTATION_VECTORS[entity->sprite.orientation],
							power * game_state->constants.ball_pass_auto_max_distance,
							game_state);
						break;
					}
					case GAME_CONTROL_PASS: {
						f32 power = PASS_POWER(time_holding_button);
						player_controls->pass = power;
						break;
					}
					case GAME_CONTROL_LONG_PASS: {
						f32 power = LONG_PASS_POWER(time_holding_button);
						if (game_controller_state->current[GAME_CONTROL_MODIFIER_LEFT].digital.down) {
							player_controls->long_pass = power;
						} else {
							player_controls->long_driven_pass = power;
						}
						break;
					}
					default:
						break;
					}
				}
			}

			break;
		}
		case PS_GOALIE: {
			if (entity->goalie.state_machine.current_state != GS_HOLD_BALL) {
				break;
			}

			// He holds the throwing pose for as long as the throw is being wound up
			player_controls->charging_throw =
				input_charge_lock_is_held(game_controller) &&
				game_controller->charge_lock.control_id == GAME_CONTROL_AUTOMATIC_PASS;

			// What he can do with the ball in his hands: put it down and carry on with his feet,
			// throw it out to a team mate, or kick it long.
			if (game_controller->charge_lock.state == INPUT_CHARGE_LOCK_CHARGED) {
				const fg_time_t time_holding_button = game_controller->charge_lock.end_time -
													  game_controller->charge_lock.start_time;
				switch (game_controller->charge_lock.control_id) {
				case GAME_CONTROL_PASS:
					player_controls->put_down = true;
					break;
				case GAME_CONTROL_AUTOMATIC_PASS: {
					f32 power = PASS_POWER(time_holding_button);
					const enum pitch_side pitch_side =
						game_state->vars.teams[entity->player.team_side].play_context.pitch_side;
					const v2 up_the_pitch = {.y = pitch_side == PSI_NORTH ? 1.0f : -1.0f};
					player_controls->pass = power;
					player_controls->automatic_pass = true;
					player_controls->pass_target_entity_id = entity_player_choose_pass_target(
						entity,
						player_controls->steering,
						up_the_pitch,
						power * game_state->constants.ball_pass_auto_max_distance,
						game_state);
					break;
				}
				case GAME_CONTROL_LONG_PASS:
					player_controls->long_pass = LONG_PASS_POWER(time_holding_button);
					break;
				default:
					break;
				}
			}
			break;
		}
		case PS_HEADER: {
			// We want to pay the price of this logic only when colliding with the ball
			if (entity->player.is_colliding_with_ball &&
				state_machine->state_data.header.type == HEADER_TYPE_PASS) {
				player_controls->pass_target_entity_id = entity_player_choose_pass_target(
					entity,
					player_controls->steering,
					ORIENTATION_VECTORS[entity->sprite.orientation],
					0,
					game_state);
			}
			break;
		}
		case PS_TACKLE:
		case PS_SLIDING_TACKLE:
		case PS_FALL:
		case PS_TACKLED_RUN:
		case PS_THROW_IN:
		case PS_COUNT:
			break;
		}
	}
}
