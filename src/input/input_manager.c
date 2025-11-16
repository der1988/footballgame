#include "input.h"
#include "game.h"
#include "gamepad.h"
#include "keyboard.h"
#include "platform/platform.h"
#include <raymath.h>

struct input_manager *input_manager_create(memory_arena_handle_t *memory_arena)
{
	assert(memory_arena);

	struct input_manager *handle = memory_arena_push(memory_arena, sizeof(*handle));

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
		struct game_controller *game_controller = &input_manager->game_controllers[i];

		switch (game_controller->type) {
		case GAME_CONTROLLER_TYPE_NONE:
			game_controller->current_state = (struct game_controller_state){0};
			continue;
		case GAME_CONTROLLER_TYPE_GAMEPAD: {
			// TODO: this needs to be fixed, i is not the id of the gamepad
			game_controller->current_state = gamepad_update(i, game_controller);
			break;
		}
		case GAME_CONTROLLER_TYPE_KEYBOARD: {
			game_controller->current_state = keyboard_update(game_controller);
			break;
		}
		}

		struct action_charge *charge = &game_controller->charge;
		if (charge->ready) {
			// Ready
			if (HAS_TIME_PASSED(game_state->time,
								charge->ended,
								game_state->constants.kick_charge_reset_time)) {
				charge->ready = charge->charging = false;
			}
		} else if (charge->charging) {
			b32 is_still_charging;
			switch (charge->type) {
			case ACT_SHOOT: {
				is_still_charging = game_controller->current_state.shoot;
				break;
			}
			case ACT_PASS: {
				is_still_charging = game_controller->current_state.pass;
				break;
			}
			case ACT_LONG_PASS: {
				is_still_charging = game_controller->current_state.long_pass;
				break;
			}
			}
			if (!is_still_charging) {
				charge->ended = game_state->time;
				charge->ready = true;
			}
		} else {
			b32 charging = false;
			enum action_charge_type charge_type;
			if (game_controller->current_state.shoot) {
				charging = true;
				charge_type = ACT_SHOOT;
			} else if (game_controller->current_state.pass) {
				charging = true;
				charge_type = ACT_PASS;
			} else if (game_controller->current_state.long_pass) {
				charging = true;
				charge_type = ACT_LONG_PASS;
			}
			if (charging) {
				charge->ready = false;
				charge->started = game_state->time;
				charge->ended = 0;
				charge->charging = true;
				charge->type = charge_type;
			}
		}
	}
}
