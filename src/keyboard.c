#include "input.h"
#include "macro.h"
#include "game_controller.h"
#include "platform.h"
#include "log.h"
#include <raymath.h>

INLINE f32 get_analogic_value_from_binding(struct keyboard_binding binding)
{
	f32 value = platform_input_device_keyboard_is_key_down(binding.to) ? 1 : 0;

	return value;
}

static void update_input_state(enum game_control game_control,
							   struct game_controller_state *state,
							   struct keyboard_binding *bindings)
{
	struct game_control_info game_control_info = game_control_infos[game_control];

	switch (game_control_info.type) {
	case GAME_CONTROL_TYPE_DIGITAL: {
		b32 down = platform_input_device_keyboard_is_key_down(bindings[game_control].to);
		state->current[game_control] =
			game_controller_digital_input_state(down, &state->previous[game_control]);
		break;
	}
	case GAME_CONTROL_TYPE_ANALOGIC:
		state->current[game_control].analogic.value =
			get_analogic_value_from_binding(bindings[game_control]);
		break;
	default:
		GLOG_ERROR(LS_GAME, "unknown game control type!");
		break;
	}
}

void keyboard_update(struct keyboard_binding *bindings, struct game_controller_state *state)
{
	for (usize i = 0; i < GAME_CONTROL_COUNT; i++) {
		update_input_state(i, state, bindings);
	}
}
