#include "input.h"
#include "macro.h"
#include "platform/platform.h"
#include <raymath.h>

static INLINE f32 get_axis_value_from_binding(struct keyboard_binding *binding)
{
	f32 value = platform_input_device_keyboard_is_key_down(binding->to) ? 1 : 0;

	return value;
}

struct game_controller_state keyboard_update(struct game_controller *game_controller)
{
	struct keyboard_binding *bindings = game_controller->keyboard.bindings;

	f32 up = get_axis_value_from_binding(&bindings[GAME_CONTROL_MOVE_UP]);
	f32 down = get_axis_value_from_binding(&bindings[GAME_CONTROL_MOVE_DOWN]);
	f32 left = get_axis_value_from_binding(&bindings[GAME_CONTROL_MOVE_LEFT]);
	f32 right = get_axis_value_from_binding(&bindings[GAME_CONTROL_MOVE_RIGHT]);

	b32 shoot = platform_input_device_keyboard_is_key_down(bindings[GAME_CONTROL_SHOOT].to);
	b32 pass = platform_input_device_keyboard_is_key_down(bindings[GAME_CONTROL_PASS].to);
	b32 long_pass = platform_input_device_keyboard_is_key_down(bindings[GAME_CONTROL_LONG_PASS].to);
	b32 tackle = platform_input_device_keyboard_is_key_down(bindings[GAME_CONTROL_TACKLE].to);

	f32 recorder_rewind = get_axis_value_from_binding(&bindings[GAME_CONTROL_RECORDER_REWIND]);
	f32 recorder_forward = get_axis_value_from_binding(&bindings[GAME_CONTROL_RECORDER_FORWARD]);
	b32 recorder_resume =
		!game_controller->previous_state.recorder_resume &&
		platform_input_device_keyboard_is_key_down(bindings[GAME_CONTROL_RECORDER_RESUME].to);

	b32 test_reset =
		!game_controller->previous_state.test_reset &&
		platform_input_device_keyboard_is_key_down(bindings[GAME_CONTROL_TEST_RESET].to);
	b32 test_crossbar =
		!game_controller->previous_state.test_crossbar &&
		platform_input_device_keyboard_is_key_down(bindings[GAME_CONTROL_TEST_CROSSBAR].to);
	b32 test_cross =
		!game_controller->previous_state.test_cross &&
		platform_input_device_keyboard_is_key_down(bindings[GAME_CONTROL_TEST_CROSS].to);
	b32 test_post = !game_controller->previous_state.test_post &&
					platform_input_device_keyboard_is_key_down(bindings[GAME_CONTROL_TEST_POST].to);
	b32 test_goalie_jump =
		!game_controller->previous_state.test_goalie_jump &&
		platform_input_device_keyboard_is_key_down(bindings[GAME_CONTROL_TEST_GOALIE_JUMP].to);

	/* This needs to be normalised. */
	v2 movement = Vector2Normalize((v2){.x = -left + right, .y = -up + down});

	return (struct game_controller_state){.movement = movement,
										  .shoot = shoot,
										  .pass = pass,
										  .long_pass = long_pass,
										  .tackle = tackle,
										  .recorder_rewind = recorder_rewind,
										  .recorder_forward = recorder_forward,
										  .recorder_resume = recorder_resume,
										  .test_reset = test_reset,
										  .test_crossbar = test_crossbar,
										  .test_cross = test_cross,
										  .test_post = test_post,
										  .test_goalie_jump = test_goalie_jump};
}
