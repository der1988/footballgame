#ifndef GAMEPAD_H
#define GAMEPAD_H

#include "input.h"

void gamepad_update(u32 gamepad_index,
					struct gamepad_binding *bindings,
					struct game_controller_state *state);

#endif // !GAMEPAD_H
