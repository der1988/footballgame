#include "game.h"
#include "input.h"
#include "math_ext.h"
#include "macro.h"

void layer_hud_draw(struct game_state *game_state)
{
	struct action_charge *charge =
		&game_state->input_manager
			 ->game_controllers[game_state->human_players[0].game_controller_id]
			 .charge;
	f32 power;
	time_t holding_time = game_state->time - charge->started;
	switch (charge->type) {
	case ACT_SHOOT: {
		power = SHOOT_POWER(holding_time);
		break;
	}
	case ACT_PASS: {
		power = PASS_POWER(holding_time);
		break;
	}
	case ACT_LONG_PASS: {
		power = LONG_PASS_POWER(holding_time);
		break;
	}
	}
	rect_t charge_box = {
		.x = 150, .y = game_state->config.screen_height - 100, .width = 300 * power, .height = 20};
	if (charge->charging || charge->ready)
		DrawRectangleRec(charge_box, power > 0.8f ? RED : WHITE);
}
