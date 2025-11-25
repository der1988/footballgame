#include "game.h"
#include "input.h"
#include "math_ext.h"
#include "macro.h"

void layer_hud_draw(struct game_state *game_state)
{
	struct game_controller *gc =
		&game_state->input_manager
			 ->game_controllers[game_state->human_players[0].game_controller_id];

	f32 power = 0;

	if (gc->charge_lock.state == INPUT_CHARGE_LOCK_CHARGING) {
		switch (gc->charge_lock.control_id) {
		case GAME_CONTROL_SHOOT:
			power = SHOOT_POWER(gc->charge_lock.duration);
			break;
		case GAME_CONTROL_PASS:
			power = PASS_POWER(gc->charge_lock.duration);
			break;
		case GAME_CONTROL_LONG_PASS:
			power = LONG_PASS_POWER(gc->charge_lock.duration);
			break;
		default:
			break;
		}
	}

	rect_t charge_box = {
		.x = 150, .y = game_state->config.screen_height - 100, .width = 300 * power, .height = 20};
	DrawRectangleRec(charge_box, power > 0.8f ? RED : WHITE);
}
