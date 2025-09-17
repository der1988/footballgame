#include "input.h"
#include "game.h"

void game_controller_charge_reset(struct game_controller *game_controller)
{
	game_controller->current_state.charge.ready = game_controller->current_state.charge.charging =
		false;
}

void game_controller_player_charge_reset(entity_id_t player_entity_id,
										 struct game_state *game_state)
{
	for (usize i = 0; i < MAX_GAME_CONTROLLERS; ++i) {
		if (game_state->human_players[i].controlled_player_entity_id == player_entity_id) {
			game_controller_charge_reset(
				&game_state->input_manager
					 ->game_controllers[game_state->human_players[i].game_controller_id]);
			break;
		}
	}
}
