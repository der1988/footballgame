#ifndef INPUT_GAMEPAD_H
#define INPUT_GAMEPAD_H

#include "input.h"

void input_gamepad_update(u32 gamepad_index,
					struct gamepad_binding *bindings,
					struct game_controller_state *state);

#endif // !INPUT_GAMEPAD_H
