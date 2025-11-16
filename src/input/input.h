#ifndef INPUT_H
#define INPUT_H

#include "memory.h"
#include "types.h"
#include <assert.h>

#define MAX_GAME_CONTROLLERS 4

#define ApplyDeadZone(value, dead_zone) (fabsf(value) < dead_zone ? 0 : value)

/* Forward declarations. */
struct game_state;

/* These are all the controls available in the game. */
enum game_control {
	/* Game controls. */
	GAME_CONTROL_MOVE_UP,
	GAME_CONTROL_MOVE_DOWN,
	GAME_CONTROL_MOVE_LEFT,
	GAME_CONTROL_MOVE_RIGHT,
	GAME_CONTROL_SHOOT,
	GAME_CONTROL_PASS,
	GAME_CONTROL_LONG_PASS,
	GAME_CONTROL_TACKLE,

	/* Recorder controls. */
	GAME_CONTROL_RECORDER_REWIND,
	GAME_CONTROL_RECORDER_FORWARD,
	GAME_CONTROL_RECORDER_RESUME,

	/* Test controls */
	GAME_CONTROL_TEST_RESET,
	GAME_CONTROL_TEST_CROSSBAR,
	GAME_CONTROL_TEST_CROSS,
	GAME_CONTROL_TEST_POST,
	GAME_CONTROL_TEST_GOALIE_JUMP,

	GAME_CONTROL_COUNT,
};

enum gamepad_binding_type {
	GAME_CONTROL_BINDING_TYPE_DIGITAL,
	GAME_CONTROL_BINDING_TYPE_ANALOGIC_POSITIVE,
	GAME_CONTROL_BINDING_TYPE_ANALOGIC_NEGATIVE,
};

/* An actual binding from a game control to a specific implementation identifier. */
struct gamepad_binding {
	enum gamepad_binding_type type;
	u32 to;
};

struct keyboard_binding {
	u32 to;
};

enum game_controller_type {
	GAME_CONTROLLER_TYPE_NONE,
	GAME_CONTROLLER_TYPE_KEYBOARD,
	GAME_CONTROLLER_TYPE_GAMEPAD,
};

/* This represents the state of a game controller during a frame. */
struct game_controller_state {
	v2 movement;

	f32 shoot;
	f32 pass;
	f32 long_pass;
	b32 tackle;

	f32 recorder_rewind;
	f32 recorder_forward;
	f32 recorder_resume;

	b32 test_reset;
	b32 test_crossbar;
	b32 test_cross;
	b32 test_post;
	b32 test_goalie_jump;
};

struct game_controller {
	enum game_controller_type type;
	union {
		struct {
			gamepad_platform_device_id_t device_id;
			struct gamepad_binding bindings[GAME_CONTROL_COUNT];
		} gamepad;
		struct {
			struct keyboard_binding bindings[GAME_CONTROL_COUNT];
		} keyboard;
	};
	struct game_controller_state current_state;
	struct game_controller_state previous_state;
	struct action_charge {
		enum action_charge_type {
			ACT_SHOOT,
			ACT_PASS,
			ACT_LONG_PASS
		} type;
		time_t started;
		time_t ended;
		b32 charging;
		b32 ready;
	} charge;
};

struct input_manager {
	struct game_controller game_controllers[MAX_GAME_CONTROLLERS];
};

struct input_manager *input_manager_create(memory_arena_handle_t *memory_arena);

void input_manager_destroy(struct input_manager *input_manager);

void input_manager_update(struct input_manager *input_manager, struct game_state *game_state);

void game_controller_charge_reset(struct game_controller *game_controller);

void game_controller_player_charge_reset(entity_id_t player_entity_id,
										 struct game_state *game_state);

#endif // !INPUT_H
