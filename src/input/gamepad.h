#ifndef GAMEPAD_H
#define GAMEPAD_H

#include "input.h"

struct game_controller_state gamepad_update(u32 gamepad_index,
											struct game_controller *game_controller);

#endif // !GAMEPAD_H
