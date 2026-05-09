#include "components.h"
#include "entities.h"
#include "e_ball.h"
#include "e_player.h"
#include "football.h"
#include "game.h"
#include "game_controller.h"
#include "input.h"
#include "macro.h"
#include <math_ext.h>
#include <raymath.h>
#include "ai.h"
#include "constants.h"
#include <string.h>

void systems_player_control_human(struct game_state *game_state, struct input *input)
{
	UNUSED(input);

	struct entity *ball_entity = game_state->vars.ball_entity;

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
		DECLARE_ENTITY_COMPONENT(entity, collider);

		struct state_machine_player *state_machine = &player->state_machine;

		struct component_player_controls *player_controls = &entity->player_controls;
		RESET_COMPONENT(player_controls);

		struct entity *last_entity_that_touched_ball =
			entity_get(game_state->entity_repository, ball->last_touch_entity_id);

		enum team_side team_idx_last_ball_possession =
			last_entity_that_touched_ball ? last_entity_that_touched_ball->player.team_side :
											TS_COUNT;

		collider->colliders[PCT_PLAYER].props.circle.radius =
			game_state->constants.player_control_radius;

		// Player is human controlled
		struct game_controller *game_controller =
			&game_state->input_manager->game_controllers[human_player->game_controller_id];
		struct game_controller_state *game_controller_state = &game_controller->state;

		player_controls->steering = calculate_movement_vector(game_controller);

		switch (state_machine->current_state) {
		case PS_NONE:
			break;
		case PS_NORMAL: {
			// Off the ball
			if (!entity->player.is_colliding_with_ball) {
				// Header
				if (game_controller_state->current[GAME_CONTROL_SHOOT].digital.pressed ||
					game_controller_state->current[GAME_CONTROL_PASS].digital.pressed) {
					if (ball_transform->virtual_height >
						game_state->constants.player_header_min_height) {
						player_controls->header_shot =
							game_controller_state->current[GAME_CONTROL_SHOOT].digital.pressed;
						player_controls->header_pass = !player_controls->header_shot;
					}
				}
				// Sliding tackle
				else if (game_controller_state->current[GAME_CONTROL_TACKLE].digital.pressed) {
					if (last_entity_that_touched_ball->header.id != entity->header.id) {
						// We don't want to trigger a tackle if we are running and we just want to make a long pass
						player_controls->sliding_tackle = true;
					}
				}
			}
			// On the ball
			else if (entity->player.is_colliding_with_ball) {
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
						player_controls->shot = power;
						break;
					}
					case GAME_CONTROL_PASS: {
						f32 power = PASS_POWER(time_holding_button);
						player_controls->pass = power;
						if (time_holding_button < 0.15f) {
							player_controls->automatic_pass = true;
						}
						break;
					}
					case GAME_CONTROL_LONG_PASS: {
						f32 power = LONG_PASS_POWER(time_holding_button);
						player_controls->long_pass = power;
						break;
					}
					default:
						break;
					}
				}
			}

			break;
		}
		case PS_SLIDING_TACKLE:
		case PS_FALL:
		case PS_HEADER:
		case PS_GOALIE:
		case PS_THROW_IN:
		case PS_COUNT:
			break;
		}
	}
}
