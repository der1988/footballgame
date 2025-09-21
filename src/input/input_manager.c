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

	handle->game_controllers[0] = (struct game_controller){
		.type = GAME_CONTROLLER_TYPE_KEYBOARD,
		.bindings = {.move_up = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
								 .to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_UP},
					 .move_down = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
								   .to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_DOWN},
					 .move_left = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
								   .to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_LEFT},
					 .move_right = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
									.to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_RIGHT},
					 .shoot = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
							   .to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_A},
					 .pass = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
							  .to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_S},
					 .long_pass = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
								   .to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_D},
					 .tackle = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
								.to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_D},

					 .recorder_rewind = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
										 .to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_1},
					 .recorder_forward = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
										  .to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_2},
					 .recorder_resume = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
										 .to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_3},
					 .test_reset = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
									.to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_4},
					 .test_cross = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
									.to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_5},
					 .test_crossbar = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
									   .to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_6},
					 .test_post = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
								   .to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_7},
					 .test_goalie_jump = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
										  .to = PLATFORM_INPUT_DEVICE_KEYBOARD_KEY_8}},
	};
	handle->game_controllers[1] = (struct game_controller){
		.type = GAME_CONTROLLER_TYPE_GAMEPAD,
		.bindings = {.move_up = {.type = GAME_CONTROL_BINDING_TYPE_ANALOGIC_NEGATIVE,
								 .to = PLATFORM_INPUT_DEVICE_GAMEPAD_AXIS_LEFT_Y},
					 .move_down = {.type = GAME_CONTROL_BINDING_TYPE_ANALOGIC_POSITIVE,
								   .to = PLATFORM_INPUT_DEVICE_GAMEPAD_AXIS_LEFT_Y},
					 .move_left = {.type = GAME_CONTROL_BINDING_TYPE_ANALOGIC_NEGATIVE,
								   .to = PLATFORM_INPUT_DEVICE_GAMEPAD_AXIS_LEFT_X},
					 .move_right = {.type = GAME_CONTROL_BINDING_TYPE_ANALOGIC_POSITIVE,
									.to = PLATFORM_INPUT_DEVICE_GAMEPAD_AXIS_LEFT_X},
					 .shoot = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
							   .to = PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_LEFT},
					 .pass = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
							  .to = PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_DOWN},
					 .long_pass = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
								   .to = PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_RIGHT},
					 .tackle = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
								.to = PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_RIGHT},

					 .recorder_rewind = {.type = GAME_CONTROL_BINDING_TYPE_ANALOGIC_POSITIVE,
										 .to = PLATFORM_INPUT_DEVICE_GAMEPAD_AXIS_LEFT_TRIGGER},
					 .recorder_forward = {.type = GAME_CONTROL_BINDING_TYPE_ANALOGIC_POSITIVE,
										  .to = PLATFORM_INPUT_DEVICE_GAMEPAD_AXIS_RIGHT_TRIGGER},
					 .recorder_resume = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
										 .to = PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_SHOULDER_RIGHT},

					 .test_reset = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
									.to = PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_SELECT},
					 .test_cross = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
									.to = PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_DPAD_DOWN},
					 .test_crossbar = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
									   .to = PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_DPAD_UP},
					 .test_post = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
								   .to = PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_DPAD_RIGHT},
					 .test_goalie_jump = {.type = GAME_CONTROL_BINDING_TYPE_DIGITAL,
										  .to = PLATFORM_INPUT_DEVICE_GAMEPAD_BUTTON_DPAD_LEFT}},
	};

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
			if (is_still_charging) {
				charge->power += 2 * game_state->dt;
				charge->power = Clamp(charge->power, 0, 1.0f);
				layer_debug_queue_draw_text(game_state->layer_debug,
											TextFormat("charge: %.2f", charge->power));
			} else {
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
				charge->direction = Vector2Normalize(game_controller->current_state.movement);
				charge->started = game_state->time;
				charge->power = 0.0f;
				charge->ended = 0;
				charge->charging = true;
				charge->type = charge_type;
			}
		}
	}
}
