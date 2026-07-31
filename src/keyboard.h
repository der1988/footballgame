#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "input.h"

void keyboard_update(struct keyboard_binding *bindings, struct game_controller_state *state);

#endif // !KEYBOARD_H
