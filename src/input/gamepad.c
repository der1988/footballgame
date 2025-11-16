#include "input.h"
#include "platform/platform.h"
#include <raymath.h>

static f32 get_axis_value_from_binding(u32 gamepad_index, struct gamepad_binding *binding)
{
	f32 value = 0;
	if (binding->type == GAME_CONTROL_BINDING_TYPE_DIGITAL) {
		value = platform_input_device_gamepad_is_button_down(gamepad_index, binding->to) ? 1 : 0;
	} else {
		value = platform_input_device_gamepad_get_axis_state(gamepad_index, binding->to);
		if (binding->type == GAME_CONTROL_BINDING_TYPE_ANALOGIC_POSITIVE && value < 0)
			value = 0;
		else if (binding->type == GAME_CONTROL_BINDING_TYPE_ANALOGIC_NEGATIVE) {
			if (value > 0)
				value = 0;
			value *= -1;
		}
	}

	return value;
}

struct game_controller_state gamepad_update(u32 gamepad_index,
											struct game_controller *game_controller)
{
	struct gamepad_binding *bindings = game_controller->gamepad.bindings;

	f32 up = get_axis_value_from_binding(gamepad_index, &bindings[GAME_CONTROL_MOVE_UP]);
	f32 down = get_axis_value_from_binding(gamepad_index, &bindings[GAME_CONTROL_MOVE_DOWN]);
	f32 left = get_axis_value_from_binding(gamepad_index, &bindings[GAME_CONTROL_MOVE_LEFT]);
	f32 right = get_axis_value_from_binding(gamepad_index, &bindings[GAME_CONTROL_MOVE_RIGHT]);

	b32 shoot = platform_input_device_gamepad_is_button_down(gamepad_index,
															 bindings[GAME_CONTROL_SHOOT].to);
	b32 pass =
		platform_input_device_gamepad_is_button_down(gamepad_index, bindings[GAME_CONTROL_PASS].to);
	b32 long_pass = platform_input_device_gamepad_is_button_down(
		gamepad_index, bindings[GAME_CONTROL_LONG_PASS].to);
	b32 tackle = platform_input_device_gamepad_is_button_down(gamepad_index,
															  bindings[GAME_CONTROL_TACKLE].to);

	f32 recorder_rewind =
		get_axis_value_from_binding(gamepad_index, &bindings[GAME_CONTROL_RECORDER_REWIND]);
	f32 recorder_forward =
		get_axis_value_from_binding(gamepad_index, &bindings[GAME_CONTROL_RECORDER_FORWARD]);
	b32 recorder_resume = !game_controller->previous_state.recorder_resume &&
						  platform_input_device_gamepad_is_button_down(
							  gamepad_index, bindings[GAME_CONTROL_RECORDER_RESUME].to);

	b32 test_reset = !game_controller->previous_state.test_reset &&
					 platform_input_device_gamepad_is_button_down(
						 gamepad_index, bindings[GAME_CONTROL_TEST_RESET].to);
	b32 test_crossbar = !game_controller->previous_state.test_crossbar &&
						platform_input_device_gamepad_is_button_down(
							gamepad_index, bindings[GAME_CONTROL_TEST_CROSSBAR].to);
	b32 test_cross = !game_controller->previous_state.test_cross &&
					 platform_input_device_gamepad_is_button_down(
						 gamepad_index, bindings[GAME_CONTROL_TEST_CROSS].to);
	b32 test_post = !game_controller->previous_state.test_post &&
					platform_input_device_gamepad_is_button_down(
						gamepad_index, bindings[GAME_CONTROL_TEST_POST].to);
	b32 test_goalie_jump = !game_controller->previous_state.test_goalie_jump &&
						   platform_input_device_gamepad_is_button_down(
							   gamepad_index, bindings[GAME_CONTROL_TEST_GOALIE_JUMP].to);

	/* Movement doesn't need to be normalised because the gamepad reading already is. */

	return (struct game_controller_state){.movement = (v2){.x = -left + right, .y = -up + down},
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
