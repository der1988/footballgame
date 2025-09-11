#include "game.h"
#include "input.h"
#include <raymath.h>

void systems_controls(struct game_state *game_state)
{
	const u8 game_constroller_idx = 0;
	struct game_controller_state *game_controller_state =
		&game_state->input_manager->game_controllers_state[game_constroller_idx];

	game_state->human_players[game_constroller_idx].controls.steering =
		Vector2ClampValue(game_controller_state->movement, -1, 1);
	struct action_charge *p1_charge =
		&game_state->human_players[game_constroller_idx].controls.charge;
	if (p1_charge->ready) {
		// Ready
		if (HAS_TIME_PASSED(
				game_state->time, p1_charge->ended, game_state->constants.kick_charge_reset_time)) {
			p1_charge->ready = p1_charge->charging = false;
		}
	} else if (p1_charge->charging) {
		b32 is_still_charging;
		switch (p1_charge->type) {
		case ACT_SHOOT: {
			is_still_charging = game_controller_state->shoot;
			break;
		}
		case ACT_PASS: {
			is_still_charging = game_controller_state->pass;
			break;
		}
		case ACT_LONG_PASS: {
			is_still_charging = game_controller_state->long_pass;
			break;
		}
		}
		if (is_still_charging) {
			p1_charge->power += 2 * game_state->dt;
			p1_charge->power = Clamp(p1_charge->power, 0, 1.0f);
			layer_debug_queue_draw_text(game_state->layer_debug,
										TextFormat("p1_charge: %.2f", p1_charge->power));
		} else {
			p1_charge->ended = game_state->time;
			p1_charge->ready = true;
		}
	} else {
		b32 charging = false;
		enum action_charge_type charge_type;
		if (game_controller_state->shoot) {
			charging = true;
			charge_type = ACT_SHOOT;
		} else if (game_controller_state->pass) {
			charging = true;
			charge_type = ACT_PASS;
		} else if (game_controller_state->long_pass) {
			charging = true;
			charge_type = ACT_LONG_PASS;
		}
		if (charging) {
			p1_charge->ready = false;
			p1_charge->direction =
				Vector2Normalize(game_state->human_players[game_constroller_idx].controls.steering);
			p1_charge->started = game_state->time;
			p1_charge->power = 0.0f;
			p1_charge->ended = 0;
			p1_charge->charging = true;
			p1_charge->type = charge_type;
		}
	}
}
