#include "game.h"

void layers_hud_draw(struct game_state *game_state)
{
	struct action_charge *p1_charge = &game_state->human_players[0].controls.charge;
	rect_t charge_box = {.x = 150,
						 .y = game_state->config.screen_height - 100,
						 .width = 300 * p1_charge->power,
						 .height = 20};
	if (p1_charge->charging || p1_charge->ready)
		DrawRectangleRec(charge_box, WHITE);
}
