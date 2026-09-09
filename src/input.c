#include "input.h"
#include "game.h"

void input_reset(struct game_controller *game_controller)
{
	/*  If we are in the middle of a charge, force the user to let go
	  before they can do anything else with this new player.*/
	if (input_charge_lock_is_held(game_controller)) {
		game_controller->charge_lock.state = INPUT_CHARGE_LOCK_WAITING_FOR_RELEASE;
	} else {
		game_controller->charge_lock.state = INPUT_CHARGE_LOCK_FREE;
	}
}

void input_reset_player(entity_id_t player_entity_id, struct game_state *game_state)
{
	for (usize i = 0; i < MAX_GAME_CONTROLLERS; ++i) {
		if (game_state->human_players[i].enabled &&
			game_state->human_players[i].controlled_player.entity_id == player_entity_id) {
			input_reset(
				&game_state->input_manager
					 ->game_controllers[game_state->human_players[i].game_controller_id]);
			break;
		}
	}
}
