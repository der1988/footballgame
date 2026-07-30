#include "input/input.h"
#include "platform/platform.h"
#include "input/game_controller.h"
#include "log/log.h"
#include "types.h"
#include <raymath.h>

static f32 get_analogic_value_from_binding(u32 gamepad_index, struct gamepad_binding binding)
{
	f32 value = 0;
	if (binding.type == GAME_CONTROL_BINDING_TYPE_DIGITAL) {
		value = platform_input_device_gamepad_is_button_down(gamepad_index, binding.to) ? 1 : 0;
	} else {
		value = platform_input_device_gamepad_get_axis_state(gamepad_index, binding.to);
		if (binding.type == GAME_CONTROL_BINDING_TYPE_ANALOGIC_POSITIVE && value < 0)
			value = 0;
		else if (binding.type == GAME_CONTROL_BINDING_TYPE_ANALOGIC_NEGATIVE) {
			if (value > 0)
				value = 0;
			value *= -1;
		}
	}

	return value;
}

static void update_input_state(platform_gamepad_device_id_t gamepad_index,
							   enum game_control game_control,
							   struct game_controller_state *state,
							   struct gamepad_binding *bindings)
{
	struct game_control_info game_control_info = game_control_infos[game_control];

	switch (game_control_info.type) {
	case GAME_CONTROL_TYPE_DIGITAL: {
		b32 down =
			platform_input_device_gamepad_is_button_down(gamepad_index, bindings[game_control].to);
		state->current[game_control] =
			get_digital_input_state(down, &state->previous[game_control]);
		break;
	}
	case GAME_CONTROL_TYPE_ANALOGIC:
		state->current[game_control].analogic.value =
			get_analogic_value_from_binding(gamepad_index, bindings[game_control]);
		break;
	default:
		GLOG_ERROR(LS_GAME, "unknown game control type!");
		break;
	}
}

void gamepad_update(u32 gamepad_index,
					struct gamepad_binding *bindings,
					struct game_controller_state *state)
{
	for (usize i = 0; i < GAME_CONTROL_COUNT; i++) {
		update_input_state(gamepad_index, i, state, bindings);
	}
}
