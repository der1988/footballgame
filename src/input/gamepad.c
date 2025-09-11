#include "input.h"
#include "platform/platform.h"
#include <raymath.h>

static f32 get_axis_value_from_binding(u32 gamepad_index, struct game_control_binding *binding)
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

static b32 get_button_state(u32 gamepad_index, struct game_control_binding *binding)
{
	assert(binding->type == GAME_CONTROL_BINDING_TYPE_DIGITAL);
	return platform_input_device_gamepad_get_button_state(gamepad_index, binding->to).down;
}

struct game_controller_state gamepad_update(u32 gamepad_index,
											struct game_controller *game_controller)
{
	struct game_controller_bindings *bindings = &game_controller->bindings;

	f32 up = get_axis_value_from_binding(gamepad_index, &bindings->move_up);
	f32 down = get_axis_value_from_binding(gamepad_index, &bindings->move_down);
	f32 left = get_axis_value_from_binding(gamepad_index, &bindings->move_left);
	f32 right = get_axis_value_from_binding(gamepad_index, &bindings->move_right);

	b32 shoot =
		platform_input_device_gamepad_get_button_state(gamepad_index, bindings->shoot.to).down;
	b32 pass =
		platform_input_device_gamepad_get_button_state(gamepad_index, bindings->pass.to).down;
	b32 long_pass =
		platform_input_device_gamepad_get_button_state(gamepad_index, bindings->long_pass.to).down;
	b32 tackle =
		platform_input_device_gamepad_get_button_state(gamepad_index, bindings->tackle.to).down;

	f32 recorder_rewind = get_axis_value_from_binding(gamepad_index, &bindings->recorder_rewind);
	f32 recorder_forward = get_axis_value_from_binding(gamepad_index, &bindings->recorder_forward);
	b32 recorder_resume =
		platform_input_device_gamepad_get_button_state(gamepad_index, bindings->recorder_resume.to)
			.pressed;

	b32 test_reset =
		platform_input_device_gamepad_get_button_state(gamepad_index, bindings->test_reset.to).down;
	b32 test_crossbar =
		platform_input_device_gamepad_get_button_state(gamepad_index, bindings->test_crossbar.to)
			.down;
	b32 test_cross =
		platform_input_device_gamepad_get_button_state(gamepad_index, bindings->test_cross.to).down;
	b32 test_post =
		platform_input_device_gamepad_get_button_state(gamepad_index, bindings->test_post.to).down;
	b32 test_goalie_jump =
		platform_input_device_gamepad_get_button_state(gamepad_index, bindings->test_goalie_jump.to)
			.down;

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
