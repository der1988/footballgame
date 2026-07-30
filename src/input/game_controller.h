#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include "types.h"
#include "macro.h"
#include "input/input.h"
#include <raymath.h>
#include <stdio.h>

INLINE struct input_state get_digital_input_state(b32 is_down_now,
												  struct input_state *previous_state)
{
	return (struct input_state){.digital.down = is_down_now,
								.digital.pressed = !previous_state->digital.down && is_down_now,
								.digital.released = previous_state->digital.down && !is_down_now};
}

INLINE v2 calculate_movement_vector(struct game_controller *game_controller)
{
	struct game_controller_state *state = &game_controller->state;

	f32 up = state->current[GAME_CONTROL_MOVE_UP].analogic.value;
	f32 down = state->current[GAME_CONTROL_MOVE_DOWN].analogic.value;
	f32 left = state->current[GAME_CONTROL_MOVE_LEFT].analogic.value;
	f32 right = state->current[GAME_CONTROL_MOVE_RIGHT].analogic.value;

	v2 movement = {.x = -left + right, .y = -up + down};

	/* Keyboard movement vector needs to be normalised because it's the sum of multiple digital inputs:
	 * the key is either down or not, so you can end up with a (-1, 1) unit vector that makes no sense.
	 * Gamepad movement vector on the other hand is already normalised.
	 */
	if (game_controller->type == GAME_CONTROLLER_TYPE_KEYBOARD) {
		movement = Vector2Normalize(movement);
	}

	return movement;
}

#endif // !GAME_CONTROLLER_H
