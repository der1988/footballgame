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
#include <string.h>
#include "ai.h"
#include "constants.h"

void systems_player_control_human(struct game_state *game_state, struct input *input)
{
	UNUSED(memset);
	UNUSED(input);

	struct entity *ball_entity = game_state->vars.ball_entity;

	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
	UNUSED(ball_physics_3d);
	UNUSED(ball_collider);
	UNUSED(ball_transform);
	UNUSED(ball);

	FOR_EACH_ENTITY(entity, game_state->entity_repository)
	{
		if (game_state->human_players[0].controlled_player_entity_id != entity_id) {
			continue;
		}

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
			&game_state->input_manager
				 ->game_controllers[game_state->human_players[0].game_controller_id];
		struct game_controller_state *game_controller_state = &game_controller->state;

		player_controls->steering = calculate_movement_vector(game_controller);

		const u8 game_controller_idx = 0;

		switch (state_machine->current_state) {
		case PS_NONE:
			break;
		case PS_NORMAL: {
			if (!entity->player.is_colliding_with_ball &&
				game_controller_state->current[GAME_CONTROL_SHOOT].digital.pressed) {
				if (ball_transform->virtual_height > 20) {
					entity_player_state_machine_change_state_to(
						&player->state_machine, PS_HEADER, game_state);
				}
			} else if (!entity->player.is_colliding_with_ball &&
					   game_controller_state->current[GAME_CONTROL_TACKLE].digital.pressed) {
				if (team_idx_last_ball_possession != player->team_side) {
					entity_player_state_machine_change_state_to(
						&player->state_machine, PS_SLIDING_TACKLE, game_state);
				}
			} else {
				/* We check the charge lock is free and the last locked game control has just been released. */
				if (game_controller->charge_lock.state == INPUT_CHARGE_LOCK_FREE &&
					game_controller->state.current[game_controller->charge_lock.control_id]
						.digital.released) {
					time_t time_holding_button = game_controller->charge_lock.duration;
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
