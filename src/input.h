#ifndef INPUT_H
#define INPUT_H

#include "types.h"

#define MAX_INPUT_DEVICES 4

// GameState forward declaration
struct game_state;

enum input_device_button_type {
	IDBT_BUTTON_START,
	IDBT_BUTTON_SELECT,
	IDBT_BUTTON_DOWN,
	IDBT_BUTTON_RIGHT,
	IDBT_BUTTON_LEFT,
	IDBT_BUTTON_UP,
	IDBT_BUTTON_TRIGGER_LEFT,
	IDBT_BUTTON_TRIGGER_RIGHT,
	IDBT_BUTTON_DPAD_DOWN,
	IDBT_BUTTON_DPAD_RIGHT,
	IDBT_BUTTON_DPAD_LEFT,
	IDBT_BUTTON_DPAD_UP,
	IDBT_BUTTON_COUNT
};

struct input_device_button {
	b32 down;
	b32 up;
	b32 pressed;
	b32 released;
};

enum input_device_type {
	IDT_KEYBOARD,
	IDT_GAMEPAD,
};

struct input_device {
	enum input_device_type type;
	v2 left_stick;
	v2 right_stick;
	f32 left_trigger;
	f32 right_trigger;
	struct input_device_button buttons[IDBT_BUTTON_COUNT];
};

struct input {
	struct input_device devices[MAX_INPUT_DEVICES];
};

struct input input_handle(struct game_state *game_state);

#endif // !INPUT_H
