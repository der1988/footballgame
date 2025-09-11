#include "../input/input.h"
#include "../platform/platform.h"
#include <raylib.h>

u32 platform_keyboard_key_mapping[] = {
	KEY_UP,	   // PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_UP
	KEY_DOWN,  // PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_DOWN
	KEY_LEFT,  // PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_LEFT
	KEY_RIGHT, // PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_RIGHT
	KEY_A,	   // PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_A
	KEY_S,	   // PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_S
	KEY_D,	   // PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_D
	KEY_Q,	   // PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_Q
	KEY_W,	   // PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_W
	KEY_E,	   // PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_E
	KEY_ONE,   // PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_1
	KEY_TWO,   // PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_2
	KEY_THREE, // PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_3
	KEY_FOUR,  // PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_4
	KEY_FIVE,  // PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_5
	KEY_SIX,   // PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_6
	KEY_SEVEN, // PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_7
	KEY_EIGHT, // PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_8
};

u32 platform_gamepad_button_mapping[] = {
	GAMEPAD_BUTTON_MIDDLE_LEFT,		 // PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_START
	GAMEPAD_BUTTON_MIDDLE_RIGHT,	 // PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_SELECT
	GAMEPAD_BUTTON_RIGHT_FACE_DOWN,	 // PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_DOWN
	GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, // PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_RIGHT
	GAMEPAD_BUTTON_RIGHT_FACE_LEFT,	 // PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_LEFT
	GAMEPAD_BUTTON_RIGHT_FACE_UP,	 // PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_UP
	GAMEPAD_BUTTON_LEFT_TRIGGER_1,	 // PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_SHOULDER_LEFT
	GAMEPAD_BUTTON_RIGHT_TRIGGER_1,	 // PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_SHOULDER_RIGHT
	GAMEPAD_BUTTON_LEFT_FACE_DOWN,	 // PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_DPAD_DOWN
	GAMEPAD_BUTTON_LEFT_FACE_RIGHT,	 // PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_DPAD_RIGHT
	GAMEPAD_BUTTON_LEFT_FACE_LEFT,	 // PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_DPAD_LEFT
	GAMEPAD_BUTTON_LEFT_FACE_UP,	 // PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_DPAD_UP
	GAMEPAD_BUTTON_LEFT_THUMB,		 // PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_LEFT_THUMB
	GAMEPAD_BUTTON_RIGHT_THUMB,		 // PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_RIGHT_THUMB
};

u32 platform_gamepad_axis_mapping[] = {
	GAMEPAD_AXIS_LEFT_X,		// PLATFORM_INPUT_DEVICE_GAMEPAD_AXIS_LEFT_X
	GAMEPAD_AXIS_LEFT_Y,		// PLATFORM_INPUT_DEVICE_GAMEPAD_AXIS_LEFT_Y
	GAMEPAD_AXIS_RIGHT_X,		// PLATFORM_INPUT_DEVICE_GAMEPAD_AXIS_RIGHT_X
	GAMEPAD_AXIS_RIGHT_Y,		// PLATFORM_INPUT_DEVICE_GAMEPAD_AXIS_RIGHT_Y
	GAMEPAD_AXIS_LEFT_TRIGGER,	// PLATFORM_INPUT_DEVICE_GAMEPAD_AXIS_LEFT_TRIGGER
	GAMEPAD_AXIS_RIGHT_TRIGGER, // PLATFORM_INPUT_DEVICE_GAMEPAD_AXIS_RIGHT_TRIGGER
};

platform_input_device_key_state_t platform_input_device_keyboard_get_key_state(const u32 input_id)
{
	u32 raylib_key_id = platform_keyboard_key_mapping[input_id];

	b32 up = IsKeyUp(raylib_key_id);
	b32 down = IsKeyDown(raylib_key_id);
	b32 pressed = IsKeyPressed(raylib_key_id);
	b32 released = IsKeyReleased(raylib_key_id);

	platform_input_device_key_state_t state = {
		.up = up, .down = down, .pressed = pressed, .released = released};

	return state;
}

b32 platform_input_device_keyboard_is_key_up(const u32 input_id)
{
	u32 raylib_key_id = platform_keyboard_key_mapping[input_id];

	return IsKeyUp(raylib_key_id);
}

b32 platform_input_device_keyboard_is_key_down(const u32 input_id)
{
	u32 raylib_key_id = platform_keyboard_key_mapping[input_id];

	return IsKeyDown(raylib_key_id);
}

b32 platform_input_device_keyboard_is_key_pressed(const u32 input_id)
{
	u32 raylib_key_id = platform_keyboard_key_mapping[input_id];

	return IsKeyPressed(raylib_key_id);
}

b32 platform_input_device_keyboard_is_key_released(const u32 input_id)
{
	u32 raylib_key_id = platform_keyboard_key_mapping[input_id];

	return IsKeyReleased(raylib_key_id);
}

platform_input_device_button_state_t
platform_input_device_gamepad_get_button_state(const u32 gamepad_id, const u32 input_id)
{
	u32 raylib_button_id = platform_gamepad_button_mapping[input_id];

	b32 up = IsGamepadButtonUp(gamepad_id, raylib_button_id);
	b32 down = IsGamepadButtonDown(gamepad_id, raylib_button_id);
	b32 pressed = IsGamepadButtonPressed(gamepad_id, raylib_button_id);
	b32 released = IsGamepadButtonReleased(gamepad_id, raylib_button_id);

	platform_input_device_button_state_t state = {
		.up = up, .down = down, .pressed = pressed, .released = released};

	return state;
}

b32 platform_input_device_gamepad_is_button_up(const u32 gamepad_id, const u32 input_id)
{
	u32 raylib_button_id = platform_gamepad_button_mapping[input_id];

	return IsGamepadButtonUp(gamepad_id, raylib_button_id);
}

b32 platform_input_device_gamepad_is_button_down(const u32 gamepad_id, const u32 input_id)
{
	u32 raylib_button_id = platform_gamepad_button_mapping[input_id];

	return IsGamepadButtonDown(gamepad_id, raylib_button_id);
}

b32 platform_input_device_gamepad_is_button_pressed(const u32 gamepad_id, const u32 input_id)
{
	u32 raylib_button_id = platform_gamepad_button_mapping[input_id];

	return IsGamepadButtonPressed(gamepad_id, raylib_button_id);
}

b32 platform_input_device_gamepad_is_button_released(const u32 gamepad_id, const u32 input_id)
{
	u32 raylib_button_id = platform_gamepad_button_mapping[input_id];

	return IsGamepadButtonReleased(gamepad_id, raylib_button_id);
}

f32 platform_input_device_gamepad_get_axis_state(const u32 gamepad_id,
												 enum platform_input_device_gamepad_axis axis)
{
	u32 raylib_axis_id = platform_gamepad_axis_mapping[axis];

	f32 value = GetGamepadAxisMovement(gamepad_id, raylib_axis_id);

	return value;
}
