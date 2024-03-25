#include "input.h"
#include "../game.h"
#include <assert.h>
#include <raylib.h>

Input input_handle(GameState *game_state) {
    i32 gamepad = 0;

    Input input = {0};

    if (IsGamepadAvailable(gamepad)) {
        input.left_stick.x = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X);
        input.left_stick.y = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y);

        input.right_stick.x = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_X);
        input.right_stick.y = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_Y);

        input.left_trigger = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_TRIGGER);
        input.right_trigger = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_RIGHT_TRIGGER);

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
    } else {
        input.left_stick.x += (IsKeyDown(KEY_LEFT) ? -1 : 0);
        input.left_stick.x += (IsKeyDown(KEY_RIGHT) ? 1 : 0);
        input.left_stick.y += (IsKeyDown(KEY_UP) ? -1 : 0);
        input.left_stick.y += (IsKeyDown(KEY_DOWN) ? 1 : 0);
        input.buttons[IBT_BUTTON_DOWN].up = IsKeyUp(KEY_Z);
        input.buttons[IBT_BUTTON_DOWN].down = IsKeyDown(KEY_Z);
        input.buttons[IBT_BUTTON_DOWN].pressed = IsKeyPressed(KEY_Z);
        input.buttons[IBT_BUTTON_DOWN].released = IsKeyReleased(KEY_Z);
    }

    return input;
}
