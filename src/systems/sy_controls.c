#include "game.h"
#include "input.h"
#include <raymath.h>

void systems_controls(struct game_state *game_state)
{
	const u8 game_constroller_idx = 0;
	struct game_controller_state *game_controller_state =
		&game_state->input_manager->game_controllers[game_constroller_idx].current_state;

	// TODO: move this logic to input_manager
}
