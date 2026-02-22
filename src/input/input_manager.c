#include "input.h"
#include "game.h"
#include "gamepad.h"
#include "keyboard.h"
#include "log.h"
#include "platform/platform.h"
#include <string.h>
#include <raymath.h>

struct game_control_info game_control_infos[] = {
	[GAME_CONTROL_MOVE_UP] = {.type = GAME_CONTROL_TYPE_ANALOGIC, .chargeable = false},
	[GAME_CONTROL_MOVE_DOWN] = {.type = GAME_CONTROL_TYPE_ANALOGIC, .chargeable = false},
	[GAME_CONTROL_MOVE_LEFT] = {.type = GAME_CONTROL_TYPE_ANALOGIC, .chargeable = false},
	[GAME_CONTROL_MOVE_RIGHT] = {.type = GAME_CONTROL_TYPE_ANALOGIC, .chargeable = false},
	[GAME_CONTROL_SHOOT] = {.type = GAME_CONTROL_TYPE_DIGITAL, .chargeable = true},
	[GAME_CONTROL_PASS] = {.type = GAME_CONTROL_TYPE_DIGITAL, .chargeable = true},
	[GAME_CONTROL_LONG_PASS] = {.type = GAME_CONTROL_TYPE_DIGITAL, .chargeable = true},
	[GAME_CONTROL_TACKLE] = {.type = GAME_CONTROL_TYPE_DIGITAL, .chargeable = false},
	[GAME_CONTROL_SWITCH_PLAYER] = {.type = GAME_CONTROL_TYPE_DIGITAL, .chargeable = false},
	[GAME_CONTROL_RECORDER_REWIND] = {.type = GAME_CONTROL_TYPE_ANALOGIC, .chargeable = false},
	[GAME_CONTROL_RECORDER_FORWARD] = {.type = GAME_CONTROL_TYPE_ANALOGIC, .chargeable = false},
	[GAME_CONTROL_RECORDER_RESUME] = {.type = GAME_CONTROL_TYPE_DIGITAL, .chargeable = false},
	[GAME_CONTROL_TEST_RESET] = {.type = GAME_CONTROL_TYPE_DIGITAL, .chargeable = false},
	[GAME_CONTROL_TEST_CROSS] = {.type = GAME_CONTROL_TYPE_DIGITAL, .chargeable = false},
	[GAME_CONTROL_TEST_CROSSBAR] = {.type = GAME_CONTROL_TYPE_DIGITAL, .chargeable = false},
	[GAME_CONTROL_TEST_POST] = {.type = GAME_CONTROL_TYPE_DIGITAL, .chargeable = false},
	[GAME_CONTROL_TEST_GOALIE_JUMP] = {.type = GAME_CONTROL_TYPE_DIGITAL, .chargeable = false},
};

struct input_manager *input_manager_create(memory_arena_handle_t *memory_arena)
{
	assert(memory_arena);

	struct input_manager *handle = memory_arena_push(memory_arena, sizeof(*handle));
	assert(handle);

	handle->game_controllers[1] = (struct game_controller){
		.type = GAME_CONTROLLER_TYPE_KEYBOARD,
		.keyboard = {
			.bindings =
				{
					[GAME_CONTROL_MOVE_UP] = {.to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_UP},
					[GAME_CONTROL_MOVE_DOWN] = {.to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_DOWN},
					[GAME_CONTROL_MOVE_LEFT] = {.to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_LEFT},
					[GAME_CONTROL_MOVE_RIGHT] = {.to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_RIGHT},
					[GAME_CONTROL_SHOOT] = {.to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_A},
					[GAME_CONTROL_PASS] = {.to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_S},
					[GAME_CONTROL_LONG_PASS] = {.to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_D},
					[GAME_CONTROL_TACKLE] = {.to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_D},

					[GAME_CONTROL_SWITCH_PLAYER] = {.to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_Q},

					[GAME_CONTROL_RECORDER_REWIND] = {.to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_1},
					[GAME_CONTROL_RECORDER_FORWARD] = {.to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_2},
					[GAME_CONTROL_RECORDER_RESUME] = {.to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_3},

					[GAME_CONTROL_TEST_RESET] = {.to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_4},
					[GAME_CONTROL_TEST_CROSS] = {.to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_5},
					[GAME_CONTROL_TEST_CROSSBAR] = {.to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_6},
					[GAME_CONTROL_TEST_POST] = {.to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_7},
					[GAME_CONTROL_TEST_GOALIE_JUMP] = {.to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_8},
				},
		}};

	handle->game_controllers[0] =
		(struct game_controller){
			.type = GAME_CONTROLLER_TYPE_GAMEPAD,
			.gamepad =
				{
					.bindings =
						{
							[GAME_CONTROL_MOVE_UP] =
								{.type = GAME_CONTROL_BINDING_TYPE_ANALOGIC_NEGATIVE,
								 .to = PLATFORM_INPUT_DEVICE_GAMEPAD_AXIS_LEFT_Y},
							[GAME_CONTROL_MOVE_DOWN] =
								{.type = GAME_CONTROL_BINDING_TYPE_ANALOGIC_POSITIVE,
								 .to = PLATFORM_INPUT_DEVICE_GAMEPAD_AXIS_LEFT_Y},
							[GAME_CONTROL_MOVE_LEFT] =
								{.type = GAME_CONTROL_BINDING_TYPE_ANALOGIC_NEGATIVE,
								 .to = PLATFORM_INPUT_DEVICE_GAMEPAD_AXIS_LEFT_X},
							[GAME_CONTROL_MOVE_RIGHT] =
								{.type = GAME_CONTROL_BINDING_TYPE_ANALOGIC_POSITIVE,
								 .to = PLATFORM_INPUT_DEVICE_GAMEPAD_AXIS_LEFT_X},
							[GAME_CONTROL_SHOOT] = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
													.to = PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_LEFT},
							[GAME_CONTROL_PASS] = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
												   .to = PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_DOWN},
							[GAME_CONTROL_LONG_PASS] =
								{.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
								 .to = PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_RIGHT},
							[GAME_CONTROL_TACKLE] = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
													 .to =
														 PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_RIGHT},

							[GAME_CONTROL_SWITCH_PLAYER] =
								{.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
								 .to = PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_SHOULDER_LEFT},

							[GAME_CONTROL_RECORDER_REWIND] =
								{.type = GAME_CONTROL_BINDING_TYPE_ANALOGIC_POSITIVE,
								 .to = PLATFORM_INPUT_DEVICE_GAMEPAD_AXIS_LEFT_TRIGGER},
							[GAME_CONTROL_RECORDER_FORWARD] =
								{.type = GAME_CONTROL_BINDING_TYPE_ANALOGIC_POSITIVE,
								 .to = PLATFORM_INPUT_DEVICE_GAMEPAD_AXIS_RIGHT_TRIGGER},
							[GAME_CONTROL_RECORDER_RESUME] =
								{.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
								 .to = PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_SHOULDER_RIGHT},

							[GAME_CONTROL_TEST_RESET] =
								{.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
								 .to = PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_SELECT},
							[GAME_CONTROL_TEST_CROSS] =
								{.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
								 .to = PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_DPAD_DOWN},
							[GAME_CONTROL_TEST_CROSSBAR] =
								{.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
								 .to = PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_DPAD_UP},
							[GAME_CONTROL_TEST_POST] =
								{.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
								 .to = PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_DPAD_RIGHT},
							[GAME_CONTROL_TEST_GOALIE_JUMP] =
								{.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
								 .to = PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_DPAD_LEFT},

						},
					.device_id = 0,
				}};

	return handle;
}

void input_manager_destroy(struct input_manager *input_manager)
{
	assert(input_manager);

	// TODO: Clean stuff
}

void input_manager_update(struct input_manager *input_manager, struct game_state *game_state)
{
	assert(input_manager);

	for (usize i = 0; i < MAX_GAME_CONTROLLERS; ++i) {
		struct game_controller *gc = &input_manager->game_controllers[i];

		/* Current state is now previous. */
		memcpy(
			gc->state.previous, gc->state.current, sizeof(struct input_state) * GAME_CONTROL_COUNT);

		switch (gc->type) {
		case GAME_CONTROLLER_TYPE_NONE:
			gc->state = (struct game_controller_state){0};
			continue;
		case GAME_CONTROLLER_TYPE_GAMEPAD: {
			gamepad_update(gc->gamepad.device_id, gc->gamepad.bindings, &gc->state);
			break;
		}
		case GAME_CONTROLLER_TYPE_KEYBOARD: {
			keyboard_update(gc->keyboard.bindings, &gc->state);
			break;
		}
		}

		switch (gc->charge_lock.state) {
		case INPUT_CHARGE_LOCK_FREE: {
			for (usize j = 0; j < GAME_CONTROL_COUNT; j++) {
				struct game_control_info *game_control_info = &game_control_infos[j];
				if (game_control_info->type == GAME_CONTROL_TYPE_DIGITAL &&
					game_control_info->chargeable && gc->state.current[j].digital.down) {
					/* Found control to charge. */
					gc->charge_lock.control_id = j;
					gc->charge_lock.state = INPUT_CHARGE_LOCK_CHARGING;
					gc->charge_lock.start_time = game_state->time;
					gc->charge_lock.end_time = 0;
					GLOGF_DEBUG(LS_GAME,
								"charging control %lu at time %.2f\n",
								j,
								gc->charge_lock.start_time);
					break;
				}
			}
			break;
		}
		case INPUT_CHARGE_LOCK_CHARGING: {
			struct game_control_info *game_control_info =
				&game_control_infos[gc->charge_lock.control_id];
			gc->charge_lock.end_time = game_state->time;
			if (game_control_info->type == GAME_CONTROL_TYPE_DIGITAL &&
				game_control_info->chargeable &&
				gc->state.current[gc->charge_lock.control_id].digital.down) {
			} else {
				//  Button released!
				gc->charge_lock.state = INPUT_CHARGE_LOCK_CHARGED;
				GLOGF_DEBUG(LS_GAME,
							"control %u released at time %.2f\n",
							gc->charge_lock.control_id,
							gc->charge_lock.end_time);
			}
			break;
		}
		case INPUT_CHARGE_LOCK_CHARGED: {
			// If we are charged for more than 500ms we reset charge.
			if ((game_state->time - gc->charge_lock.end_time) > 1.0f) {
				gc->charge_lock.state = INPUT_CHARGE_LOCK_FREE;
				GLOGF_DEBUG(LS_GAME, "control %u charge timed out\n", gc->charge_lock.control_id);
			}
			break;
		}
		case INPUT_CHARGE_LOCK_WAITING_FOR_RELEASE: {
			struct game_control_info *game_control_info =
				&game_control_infos[gc->charge_lock.control_id];
			if (game_control_info->type == GAME_CONTROL_TYPE_DIGITAL &&
				game_control_info->chargeable &&
				!gc->state.current[gc->charge_lock.control_id].digital.down) {
				gc->charge_lock.state = INPUT_CHARGE_LOCK_FREE;
				GLOGF_DEBUG(LS_GAME, "control %u released\n", gc->charge_lock.control_id);
			}
			break;
		}
		}
	}
}
