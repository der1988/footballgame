#include "input.h"
#include "game.h"
#include <math.h>
#include <raylib.h>

#define ApplyDeadZone(value, dead_zone) (fabsf(value) < dead_zone ? 0 : value)

struct input input_handle(struct game_state *game_state)
{
    i32 gamepad = 0;

    struct input input = {0};

    if (IsGamepadAvailable(gamepad))
    {
	input.left_stick.x = ApplyDeadZone(GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X),
					   game_state->constants.input_stick_dead_zone);
	input.left_stick.y = ApplyDeadZone(GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y),
					   game_state->constants.input_stick_dead_zone);

	input.right_stick.x = ApplyDeadZone(GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_X),
					    game_state->constants.input_stick_dead_zone);
	input.right_stick.y = ApplyDeadZone(GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_Y),
					    game_state->constants.input_stick_dead_zone);

	input.left_trigger = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_TRIGGER);
	input.right_trigger = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_TRIGGER);

	input.buttons[IBT_BUTTON_START].up =
	    IsGamepadButtonUp(gamepad, GAMEPAD_BUTTON_MIDDLE_RIGHT);
	input.buttons[IBT_BUTTON_START].down =
	    IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_MIDDLE_RIGHT);
	input.buttons[IBT_BUTTON_START].pressed =
	    IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_MIDDLE_RIGHT);
	input.buttons[IBT_BUTTON_START].released =
	    IsGamepadButtonReleased(gamepad, GAMEPAD_BUTTON_MIDDLE_RIGHT);

	input.buttons[IBT_BUTTON_SELECT].up =
	    IsGamepadButtonUp(gamepad, GAMEPAD_BUTTON_MIDDLE_LEFT);
	input.buttons[IBT_BUTTON_SELECT].down =
	    IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_MIDDLE_LEFT);
	input.buttons[IBT_BUTTON_SELECT].pressed =
	    IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_MIDDLE_LEFT);
	input.buttons[IBT_BUTTON_SELECT].released =
	    IsGamepadButtonReleased(gamepad, GAMEPAD_BUTTON_MIDDLE_LEFT);

	input.buttons[IBT_BUTTON_DOWN].up =
	    IsGamepadButtonUp(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
	input.buttons[IBT_BUTTON_DOWN].down =
	    IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
	input.buttons[IBT_BUTTON_DOWN].pressed =
	    IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
	input.buttons[IBT_BUTTON_DOWN].released =
	    IsGamepadButtonReleased(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);

	input.buttons[IBT_BUTTON_UP].up = IsGamepadButtonUp(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_UP);
	input.buttons[IBT_BUTTON_UP].down =
	    IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_UP);
	input.buttons[IBT_BUTTON_UP].pressed =
	    IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_UP);
	input.buttons[IBT_BUTTON_UP].released =
	    IsGamepadButtonReleased(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_UP);

	input.buttons[IBT_BUTTON_LEFT].up =
	    IsGamepadButtonUp(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
	input.buttons[IBT_BUTTON_LEFT].down =
	    IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
	input.buttons[IBT_BUTTON_LEFT].pressed =
	    IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
	input.buttons[IBT_BUTTON_LEFT].released =
	    IsGamepadButtonReleased(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT);

	input.buttons[IBT_BUTTON_RIGHT].up =
	    IsGamepadButtonUp(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
	input.buttons[IBT_BUTTON_RIGHT].down =
	    IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
	input.buttons[IBT_BUTTON_RIGHT].pressed =
	    IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
	input.buttons[IBT_BUTTON_RIGHT].released =
	    IsGamepadButtonReleased(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);

	input.buttons[IBT_BUTTON_TRIGGER_LEFT].up =
	    IsGamepadButtonUp(gamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_1);
	input.buttons[IBT_BUTTON_TRIGGER_LEFT].down =
	    IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_1);
	input.buttons[IBT_BUTTON_TRIGGER_LEFT].pressed =
	    IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_1);
	input.buttons[IBT_BUTTON_TRIGGER_LEFT].released =
	    IsGamepadButtonReleased(gamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_1);

	input.buttons[IBT_BUTTON_TRIGGER_RIGHT].up =
	    IsGamepadButtonUp(gamepad, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
	input.buttons[IBT_BUTTON_TRIGGER_RIGHT].down =
	    IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
	input.buttons[IBT_BUTTON_TRIGGER_RIGHT].pressed =
	    IsGamepadButtonPressed(gamepad, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
	input.buttons[IBT_BUTTON_TRIGGER_RIGHT].released =
	    IsGamepadButtonReleased(gamepad, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
    }
    else
    {
	input.left_stick.x += (IsKeyDown(KEY_LEFT) ? -1 : 0);
	input.left_stick.x += (IsKeyDown(KEY_RIGHT) ? 1 : 0);
	input.left_stick.y += (IsKeyDown(KEY_UP) ? -1 : 0);
	input.left_stick.y += (IsKeyDown(KEY_DOWN) ? 1 : 0);
	input.buttons[IBT_BUTTON_START].up = IsKeyUp(KEY_ONE);
	input.buttons[IBT_BUTTON_START].down = IsKeyDown(KEY_ONE);
	input.buttons[IBT_BUTTON_START].pressed = IsKeyPressed(KEY_ONE);
	input.buttons[IBT_BUTTON_START].released = IsKeyReleased(KEY_ONE);
	input.buttons[IBT_BUTTON_SELECT].up = IsKeyUp(KEY_TWO);
	input.buttons[IBT_BUTTON_SELECT].down = IsKeyDown(KEY_TWO);
	input.buttons[IBT_BUTTON_SELECT].pressed = IsKeyPressed(KEY_TWO);
	input.buttons[IBT_BUTTON_SELECT].released = IsKeyReleased(KEY_TWO);
	input.buttons[IBT_BUTTON_LEFT].up = IsKeyUp(KEY_A);
	input.buttons[IBT_BUTTON_LEFT].down = IsKeyDown(KEY_A);
	input.buttons[IBT_BUTTON_LEFT].pressed = IsKeyPressed(KEY_A);
	input.buttons[IBT_BUTTON_LEFT].released = IsKeyReleased(KEY_A);
	input.buttons[IBT_BUTTON_RIGHT].up = IsKeyUp(KEY_D);
	input.buttons[IBT_BUTTON_RIGHT].down = IsKeyDown(KEY_D);
	input.buttons[IBT_BUTTON_RIGHT].pressed = IsKeyPressed(KEY_D);
	input.buttons[IBT_BUTTON_RIGHT].released = IsKeyReleased(KEY_D);
	input.buttons[IBT_BUTTON_UP].up = IsKeyUp(KEY_W);
	input.buttons[IBT_BUTTON_UP].down = IsKeyDown(KEY_W);
	input.buttons[IBT_BUTTON_UP].pressed = IsKeyPressed(KEY_W);
	input.buttons[IBT_BUTTON_UP].released = IsKeyReleased(KEY_W);
	input.buttons[IBT_BUTTON_DOWN].up = IsKeyUp(KEY_S);
	input.buttons[IBT_BUTTON_DOWN].down = IsKeyDown(KEY_S);
	input.buttons[IBT_BUTTON_DOWN].pressed = IsKeyPressed(KEY_S);
	input.buttons[IBT_BUTTON_DOWN].released = IsKeyReleased(KEY_S);
	input.buttons[IBT_BUTTON_TRIGGER_LEFT].up = IsKeyUp(KEY_Q);
	input.buttons[IBT_BUTTON_TRIGGER_LEFT].down = IsKeyDown(KEY_Q);
	input.buttons[IBT_BUTTON_TRIGGER_LEFT].pressed = IsKeyPressed(KEY_Q);
	input.buttons[IBT_BUTTON_TRIGGER_LEFT].released = IsKeyReleased(KEY_Q);
	input.buttons[IBT_BUTTON_TRIGGER_RIGHT].up = IsKeyUp(KEY_E);
	input.buttons[IBT_BUTTON_TRIGGER_RIGHT].down = IsKeyDown(KEY_E);
	input.buttons[IBT_BUTTON_TRIGGER_RIGHT].pressed = IsKeyPressed(KEY_E);
	input.buttons[IBT_BUTTON_TRIGGER_RIGHT].released = IsKeyReleased(KEY_E);
    }

    return input;
}
