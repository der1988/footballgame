#include "input.h"
#include "macro.h"
#include "platform/platform.h"
#include <raymath.h>

static INLINE f32 get_axis_value_from_binding(struct game_control_binding *binding)
{
	assert(binding->type == GAME_CONTROL_BINDING_TYPE_DIGITAL);

	f32 value = platform_input_device_keyboard_is_key_down(binding->to) ? 1 : 0;

	return value;
}

struct game_controller_state keyboard_update(struct game_controller *game_controller)
{
	struct game_controller_bindings *bindings = &game_controller->bindings;

	f32 up = get_axis_value_from_binding(&bindings->move_up);
	f32 down = get_axis_value_from_binding(&bindings->move_down);
	f32 left = get_axis_value_from_binding(&bindings->move_left);
	f32 right = get_axis_value_from_binding(&bindings->move_right);

	b32 shoot = platform_input_device_keyboard_is_key_down(bindings->shoot.to);
	b32 pass = platform_input_device_keyboard_is_key_down(bindings->pass.to);
	b32 long_pass = platform_input_device_keyboard_is_key_down(bindings->long_pass.to);
	b32 tackle = platform_input_device_keyboard_is_key_down(bindings->tackle.to);

	f32 recorder_rewind = get_axis_value_from_binding(&bindings->recorder_rewind);
	f32 recorder_forward = get_axis_value_from_binding(&bindings->recorder_forward);
	b32 recorder_resume = platform_input_device_keyboard_is_key_down(bindings->recorder_resume.to);

	b32 test_reset = platform_input_device_keyboard_is_key_pressed(bindings->test_reset.to);
	b32 test_crossbar = platform_input_device_keyboard_is_key_pressed(bindings->test_crossbar.to);
	b32 test_cross = platform_input_device_keyboard_is_key_pressed(bindings->test_cross.to);
	b32 test_post = platform_input_device_keyboard_is_key_pressed(bindings->test_post.to);
	b32 test_goalie_jump =
		platform_input_device_keyboard_is_key_pressed(bindings->test_goalie_jump.to);

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
