#include "input.h"
#include "game.h"
#include <math.h>

#define ApplyDeadZone(value, dead_zone) (fabsf(value) < dead_zone ? 0 : value)

struct input input_handle(struct game_state *game_state)
{
	struct input input = {0};

	for (usize i = 0; i < MAX_INPUT_DEVICES; ++i) {
		if (IsGamepadAvailable(i)) {
			input.devices[i].type = IDT_GAMEPAD;

			input.devices[i].left_stick.x =
				ApplyDeadZone(GetGamepadAxisMovement(i, GAMEPAD_AXIS_LEFT_X),
							  game_state->constants.input_stick_dead_zone);
			input.devices[i].left_stick.y =
				ApplyDeadZone(GetGamepadAxisMovement(i, GAMEPAD_AXIS_LEFT_Y),
							  game_state->constants.input_stick_dead_zone);

			input.devices[i].right_stick.x =
				ApplyDeadZone(GetGamepadAxisMovement(i, GAMEPAD_AXIS_RIGHT_X),
							  game_state->constants.input_stick_dead_zone);
			input.devices[i].right_stick.y =
				ApplyDeadZone(GetGamepadAxisMovement(i, GAMEPAD_AXIS_RIGHT_Y),
							  game_state->constants.input_stick_dead_zone);

			input.devices[i].left_trigger = GetGamepadAxisMovement(i, GAMEPAD_AXIS_LEFT_TRIGGER);
			input.devices[i].right_trigger = GetGamepadAxisMovement(i, GAMEPAD_AXIS_RIGHT_TRIGGER);

			input.devices[i].buttons[IDBT_BUTTON_START].up =
				IsGamepadButtonUp(i, GAMEPAD_BUTTON_MIDDLE_RIGHT);
			input.devices[i].buttons[IDBT_BUTTON_START].down =
				IsGamepadButtonDown(i, GAMEPAD_BUTTON_MIDDLE_RIGHT);
			input.devices[i].buttons[IDBT_BUTTON_START].pressed =
				IsGamepadButtonPressed(i, GAMEPAD_BUTTON_MIDDLE_RIGHT);
			input.devices[i].buttons[IDBT_BUTTON_START].released =
				IsGamepadButtonReleased(i, GAMEPAD_BUTTON_MIDDLE_RIGHT);

			input.devices[i].buttons[IDBT_BUTTON_SELECT].up =
				IsGamepadButtonUp(i, GAMEPAD_BUTTON_MIDDLE_LEFT);
			input.devices[i].buttons[IDBT_BUTTON_SELECT].down =
				IsGamepadButtonDown(i, GAMEPAD_BUTTON_MIDDLE_LEFT);
			input.devices[i].buttons[IDBT_BUTTON_SELECT].pressed =
				IsGamepadButtonPressed(i, GAMEPAD_BUTTON_MIDDLE_LEFT);
			input.devices[i].buttons[IDBT_BUTTON_SELECT].released =
				IsGamepadButtonReleased(i, GAMEPAD_BUTTON_MIDDLE_LEFT);

			input.devices[i].buttons[IDBT_BUTTON_DOWN].up =
				IsGamepadButtonUp(i, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
			input.devices[i].buttons[IDBT_BUTTON_DOWN].down =
				IsGamepadButtonDown(i, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
			input.devices[i].buttons[IDBT_BUTTON_DOWN].pressed =
				IsGamepadButtonPressed(i, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
			input.devices[i].buttons[IDBT_BUTTON_DOWN].released =
				IsGamepadButtonReleased(i, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);

			input.devices[i].buttons[IDBT_BUTTON_UP].up =
				IsGamepadButtonUp(i, GAMEPAD_BUTTON_RIGHT_FACE_UP);
			input.devices[i].buttons[IDBT_BUTTON_UP].down =
				IsGamepadButtonDown(i, GAMEPAD_BUTTON_RIGHT_FACE_UP);
			input.devices[i].buttons[IDBT_BUTTON_UP].pressed =
				IsGamepadButtonPressed(i, GAMEPAD_BUTTON_RIGHT_FACE_UP);
			input.devices[i].buttons[IDBT_BUTTON_UP].released =
				IsGamepadButtonReleased(i, GAMEPAD_BUTTON_RIGHT_FACE_UP);

			input.devices[i].buttons[IDBT_BUTTON_LEFT].up =
				IsGamepadButtonUp(i, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
			input.devices[i].buttons[IDBT_BUTTON_LEFT].down =
				IsGamepadButtonDown(i, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
			input.devices[i].buttons[IDBT_BUTTON_LEFT].pressed =
				IsGamepadButtonPressed(i, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
			input.devices[i].buttons[IDBT_BUTTON_LEFT].released =
				IsGamepadButtonReleased(i, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);

			input.devices[i].buttons[IDBT_BUTTON_RIGHT].up =
				IsGamepadButtonUp(i, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
			input.devices[i].buttons[IDBT_BUTTON_RIGHT].down =
				IsGamepadButtonDown(i, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
			input.devices[i].buttons[IDBT_BUTTON_RIGHT].pressed =
				IsGamepadButtonPressed(i, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
			input.devices[i].buttons[IDBT_BUTTON_RIGHT].released =
				IsGamepadButtonReleased(i, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);

			input.devices[i].buttons[IDBT_BUTTON_DPAD_DOWN].up =
				IsGamepadButtonUp(i, GAMEPAD_BUTTON_LEFT_FACE_DOWN);
			input.devices[i].buttons[IDBT_BUTTON_DPAD_DOWN].down =
				IsGamepadButtonDown(i, GAMEPAD_BUTTON_LEFT_FACE_DOWN);
			input.devices[i].buttons[IDBT_BUTTON_DPAD_DOWN].pressed =
				IsGamepadButtonPressed(i, GAMEPAD_BUTTON_LEFT_FACE_DOWN);
			input.devices[i].buttons[IDBT_BUTTON_DPAD_DOWN].released =
				IsGamepadButtonReleased(i, GAMEPAD_BUTTON_LEFT_FACE_DOWN);

			input.devices[i].buttons[IDBT_BUTTON_DPAD_UP].up =
				IsGamepadButtonUp(i, GAMEPAD_BUTTON_LEFT_FACE_UP);
			input.devices[i].buttons[IDBT_BUTTON_DPAD_UP].down =
				IsGamepadButtonDown(i, GAMEPAD_BUTTON_LEFT_FACE_UP);
			input.devices[i].buttons[IDBT_BUTTON_DPAD_UP].pressed =
				IsGamepadButtonPressed(i, GAMEPAD_BUTTON_LEFT_FACE_UP);
			input.devices[i].buttons[IDBT_BUTTON_DPAD_UP].released =
				IsGamepadButtonReleased(i, GAMEPAD_BUTTON_LEFT_FACE_UP);

			input.devices[i].buttons[IDBT_BUTTON_DPAD_LEFT].up =
				IsGamepadButtonUp(i, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
			input.devices[i].buttons[IDBT_BUTTON_DPAD_LEFT].down =
				IsGamepadButtonDown(i, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
			input.devices[i].buttons[IDBT_BUTTON_DPAD_LEFT].pressed =
				IsGamepadButtonPressed(i, GAMEPAD_BUTTON_LEFT_FACE_LEFT);
			input.devices[i].buttons[IDBT_BUTTON_DPAD_LEFT].released =
				IsGamepadButtonReleased(i, GAMEPAD_BUTTON_LEFT_FACE_LEFT);

			input.devices[i].buttons[IDBT_BUTTON_DPAD_RIGHT].up =
				IsGamepadButtonUp(i, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
			input.devices[i].buttons[IDBT_BUTTON_DPAD_RIGHT].down =
				IsGamepadButtonDown(i, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
			input.devices[i].buttons[IDBT_BUTTON_DPAD_RIGHT].pressed =
				IsGamepadButtonPressed(i, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
			input.devices[i].buttons[IDBT_BUTTON_DPAD_RIGHT].released =
				IsGamepadButtonReleased(i, GAMEPAD_BUTTON_LEFT_FACE_RIGHT);

			input.devices[i].buttons[IDBT_BUTTON_TRIGGER_LEFT].up =
				IsGamepadButtonUp(i, GAMEPAD_BUTTON_LEFT_TRIGGER_1);
			input.devices[i].buttons[IDBT_BUTTON_TRIGGER_LEFT].down =
				IsGamepadButtonDown(i, GAMEPAD_BUTTON_LEFT_TRIGGER_1);
			input.devices[i].buttons[IDBT_BUTTON_TRIGGER_LEFT].pressed =
				IsGamepadButtonPressed(i, GAMEPAD_BUTTON_LEFT_TRIGGER_1);
			input.devices[i].buttons[IDBT_BUTTON_TRIGGER_LEFT].released =
				IsGamepadButtonReleased(i, GAMEPAD_BUTTON_LEFT_TRIGGER_1);

			input.devices[i].buttons[IDBT_BUTTON_TRIGGER_RIGHT].up =
				IsGamepadButtonUp(i, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
			input.devices[i].buttons[IDBT_BUTTON_TRIGGER_RIGHT].down =
				IsGamepadButtonDown(i, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
			input.devices[i].buttons[IDBT_BUTTON_TRIGGER_RIGHT].pressed =
				IsGamepadButtonPressed(i, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
			input.devices[i].buttons[IDBT_BUTTON_TRIGGER_RIGHT].released =
				IsGamepadButtonReleased(i, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
		} else if (i == 0) {
			// If we don't have any gamepad let's use keys
			// TODO: this should be done better
			input.devices[i].type = IDT_KEYBOARD;

			input.devices[i].left_stick.x += (IsKeyDown(KEY_LEFT) ? -1 : 0);
			input.devices[i].left_stick.x += (IsKeyDown(KEY_RIGHT) ? 1 : 0);
			input.devices[i].left_stick.y += (IsKeyDown(KEY_UP) ? -1 : 0);
			input.devices[i].left_stick.y += (IsKeyDown(KEY_DOWN) ? 1 : 0);
			input.devices[i].buttons[IDBT_BUTTON_START].up = IsKeyUp(KEY_ONE);
			input.devices[i].buttons[IDBT_BUTTON_START].down = IsKeyDown(KEY_ONE);
			input.devices[i].buttons[IDBT_BUTTON_START].pressed = IsKeyPressed(KEY_ONE);
			input.devices[i].buttons[IDBT_BUTTON_START].released = IsKeyReleased(KEY_ONE);
			input.devices[i].buttons[IDBT_BUTTON_SELECT].up = IsKeyUp(KEY_TWO);
			input.devices[i].buttons[IDBT_BUTTON_SELECT].down = IsKeyDown(KEY_TWO);
			input.devices[i].buttons[IDBT_BUTTON_SELECT].pressed = IsKeyPressed(KEY_TWO);
			input.devices[i].buttons[IDBT_BUTTON_SELECT].released = IsKeyReleased(KEY_TWO);
			input.devices[i].buttons[IDBT_BUTTON_LEFT].up = IsKeyUp(KEY_A);
			input.devices[i].buttons[IDBT_BUTTON_LEFT].down = IsKeyDown(KEY_A);
			input.devices[i].buttons[IDBT_BUTTON_LEFT].pressed = IsKeyPressed(KEY_A);
			input.devices[i].buttons[IDBT_BUTTON_LEFT].released = IsKeyReleased(KEY_A);
			input.devices[i].buttons[IDBT_BUTTON_RIGHT].up = IsKeyUp(KEY_D);
			input.devices[i].buttons[IDBT_BUTTON_RIGHT].down = IsKeyDown(KEY_D);
			input.devices[i].buttons[IDBT_BUTTON_RIGHT].pressed = IsKeyPressed(KEY_D);
			input.devices[i].buttons[IDBT_BUTTON_RIGHT].released = IsKeyReleased(KEY_D);
			input.devices[i].buttons[IDBT_BUTTON_UP].up = IsKeyUp(KEY_W);
			input.devices[i].buttons[IDBT_BUTTON_UP].down = IsKeyDown(KEY_W);
			input.devices[i].buttons[IDBT_BUTTON_UP].pressed = IsKeyPressed(KEY_W);
			input.devices[i].buttons[IDBT_BUTTON_UP].released = IsKeyReleased(KEY_W);
			input.devices[i].buttons[IDBT_BUTTON_DOWN].up = IsKeyUp(KEY_S);
			input.devices[i].buttons[IDBT_BUTTON_DOWN].down = IsKeyDown(KEY_S);
			input.devices[i].buttons[IDBT_BUTTON_DOWN].pressed = IsKeyPressed(KEY_S);
			input.devices[i].buttons[IDBT_BUTTON_DOWN].released = IsKeyReleased(KEY_S);
			input.devices[i].buttons[IDBT_BUTTON_TRIGGER_LEFT].up = IsKeyUp(KEY_Q);
			input.devices[i].buttons[IDBT_BUTTON_TRIGGER_LEFT].down = IsKeyDown(KEY_Q);
			input.devices[i].buttons[IDBT_BUTTON_TRIGGER_LEFT].pressed = IsKeyPressed(KEY_Q);
			input.devices[i].buttons[IDBT_BUTTON_TRIGGER_LEFT].released = IsKeyReleased(KEY_Q);
			input.devices[i].buttons[IDBT_BUTTON_TRIGGER_RIGHT].up = IsKeyUp(KEY_E);
			input.devices[i].buttons[IDBT_BUTTON_TRIGGER_RIGHT].down = IsKeyDown(KEY_E);
			input.devices[i].buttons[IDBT_BUTTON_TRIGGER_RIGHT].pressed = IsKeyPressed(KEY_E);
			input.devices[i].buttons[IDBT_BUTTON_TRIGGER_RIGHT].released = IsKeyReleased(KEY_E);
		}
	}

	return input;
}
