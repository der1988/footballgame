#include "game.h"

void layer_hud_draw(struct game_state *game_state)
{
	struct action_charge *charge =
		&game_state->input_manager
			 ->game_controllers[game_state->human_players[0].game_controller_id]
			 .current_state.charge;
	rect_t charge_box = {.x = 150,
						 .y = game_state->config.screen_height - 100,
						 .width = 300 * charge->power,
						 .height = 20};
	if (charge->charging || charge->ready)
		DrawRectangleRec(charge_box, charge->power > 0.8f ? RED : WHITE);
}
