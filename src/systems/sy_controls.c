#include "../game.h"
#include "input.h"
#include "layers/layers.h"
#include "utils.h"
#include <raylib.h>
#include <raymath.h>

void systems_controls(GameState *game_state, Input *input) {
    game_state->vars.p1.controls.normalised_direction = Vector2ClampValue(input->left_stick, -1, 1);
    game_state->vars.p1.controls.sprint = input->buttons[IBT_BUTTON_TRIGGER_RIGHT].down;
    ActionCharge *p1_charge = &game_state->vars.p1.controls.charge;
    if (p1_charge->ready) {
        // Ready
        if (HAS_TIME_PASSED(game_state->time, p1_charge->ended,
                            game_state->constants.kick_charge_reset_time)) {
            p1_charge->ready = p1_charge->charging = false;
        }
    } else if (p1_charge->charging) {
        enum InputButtonType button_type;
        switch (p1_charge->type) {
        case ACT_SHOOT: {
            button_type = IBT_BUTTON_LEFT;
            break;
        }
        case ACT_PASS: {
            button_type = IBT_BUTTON_DOWN;
            break;
        }
        case ACT_LONG_PASS: {
            button_type = IBT_BUTTON_RIGHT;
            break;
        }
        }
        if (input->buttons[button_type].down) {
            p1_charge->power += 1 * game_state->dt;
            p1_charge->power = Clamp(p1_charge->power, 0, 1.0f);
            layers_debug_queue_draw_text(game_state->layer_debug,
                                         TextFormat("p1_charge: %.2f", p1_charge->power));
        } else {
            p1_charge->ended = game_state->time;
            p1_charge->ready = true;
        }
    } else {
        b32 charging = false;
        ActionChargeType charge_type;
        if (input->buttons[IBT_BUTTON_LEFT].down) {
            charging = true;
            charge_type = ACT_SHOOT;
        } else if (input->buttons[IBT_BUTTON_DOWN].down) {
            charging = true;
            charge_type = ACT_PASS;
        } else if (input->buttons[IBT_BUTTON_RIGHT].down) {
            charging = true;
            charge_type = ACT_LONG_PASS;
        }
        if (charging) {
            p1_charge->ready = false;
            p1_charge->normalised_direction = game_state->vars.p1.controls.normalised_direction;
            p1_charge->started = game_state->time;
            p1_charge->power = 0.0f;
            p1_charge->ended = 0;
            p1_charge->charging = true;
            p1_charge->type = charge_type;
        }
    }
}
