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

enum game_control_type {
	GAME_CONTROL_TYPE_ANALOGIC,
	GAME_CONTROL_TYPE_DIGITAL,
};

struct game_control_info {
	enum game_control_type type;
	b32 chargeable;
};

extern struct game_control_info game_control_infos[];

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

struct input_state {
	union {
		struct {
			f32 value;
		} analogic;
		struct {
			b32 down;
			b32 pressed;
			b32 released;
		} digital;
	};
};

enum input_charge_lock_state {
	INPUT_CHARGE_LOCK_FREE,
	INPUT_CHARGE_LOCK_CHARGING,
	INPUT_CHARGE_LOCK_CHARGED,
	INPUT_CHARGE_LOCK_WAITING_FOR_RELEASE,
};

enum game_controller_type {
	GAME_CONTROLLER_TYPE_NONE,
	GAME_CONTROLLER_TYPE_KEYBOARD,
	GAME_CONTROLLER_TYPE_GAMEPAD,
};

struct game_controller_state {
	struct input_state current[GAME_CONTROL_COUNT];
	struct input_state previous[GAME_CONTROL_COUNT];
};

struct game_controller {
	enum game_controller_type type;
	union {
		struct {
			platform_gamepad_device_id_t device_id;
			struct gamepad_binding bindings[GAME_CONTROL_COUNT];
		} gamepad;
		struct {
			struct keyboard_binding bindings[GAME_CONTROL_COUNT];
		} keyboard;
	};
	struct game_controller_state state;

	struct {
		enum input_charge_lock_state state;
		enum game_control control_id;
		fg_time_t start_time;
		fg_time_t end_time;
	} charge_lock;
};

struct input_manager {
	struct game_controller game_controllers[MAX_GAME_CONTROLLERS];
};

struct input_manager *input_manager_create(memory_arena_handle_t *memory_arena);

void input_manager_destroy(struct input_manager *input_manager);

void input_manager_update(struct input_manager *input_manager, struct game_state *game_state);

void game_controller_reset(struct game_controller *game_controller);

void game_controller_player_reset(entity_id_t player_entity_id, struct game_state *game_state);

#endif // !INPUT_H
