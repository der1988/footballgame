#include "game.h"
#include "input.h"
#include <raymath.h>

void systems_controls(struct game_state *game_state, struct input *input)
{
    game_state->human_players[0].controls.steering =
	Vector2ClampValue(input->devices[0].left_stick, -1, 1);
    struct action_charge *p1_charge = &game_state->human_players[0].controls.charge;
    if (p1_charge->ready) {
	// Ready
	if (HAS_TIME_PASSED(
		game_state->time, p1_charge->ended, game_state->constants.kick_charge_reset_time)) {
	    p1_charge->ready = p1_charge->charging = false;
	}
    } else if (p1_charge->charging) {
	enum input_device_button_type button_type;
	switch (p1_charge->type) {
	case ACT_SHOOT: {
	    button_type = IDBT_BUTTON_LEFT;
	    break;
	}
	case ACT_PASS: {
	    button_type = IDBT_BUTTON_DOWN;
	    break;
	}
	case ACT_LONG_PASS: {
	    button_type = IDBT_BUTTON_RIGHT;
	    break;
	}
	}
	if (input->devices[0].buttons[button_type].down) {
	    p1_charge->power += 2 * game_state->dt;
	    p1_charge->power = Clamp(p1_charge->power, 0, 1.0f);
	    layers_debug_queue_draw_text(game_state->layer_debug,
					 TextFormat("p1_charge: %.2f", p1_charge->power));
	} else {
	    p1_charge->ended = game_state->time;
	    p1_charge->ready = true;
	}
    } else {
	b32 charging = false;
	enum action_charge_type charge_type;
	if (input->devices[0].buttons[IDBT_BUTTON_LEFT].down) {
	    charging = true;
	    charge_type = ACT_SHOOT;
	} else if (input->devices[0].buttons[IDBT_BUTTON_DOWN].down) {
	    charging = true;
	    charge_type = ACT_PASS;
	} else if (input->devices[0].buttons[IDBT_BUTTON_RIGHT].down) {
	    charging = true;
	    charge_type = ACT_LONG_PASS;
	}
	if (charging) {
	    p1_charge->ready = false;
	    p1_charge->direction = Vector2Normalize(game_state->human_players[0].controls.steering);
	    p1_charge->started = game_state->time;
	    p1_charge->power = 0.0f;
	    p1_charge->ended = 0;
	    p1_charge->charging = true;
	    p1_charge->type = charge_type;
	}
    }
}
