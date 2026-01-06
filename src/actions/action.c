#include "action.h"
#include "game.h"
#include "entities.h"

const char *ActionTypeToString[AT_COUNT] = {"change_color_from_to",
											"play_animation",
											"sequence",
											"spawn",
											"tween_float_from_to",
											"rotate_to",
											"toggle_boolean"};

void action_run(struct action_runner *action_runner, entity_id_t id, struct game_state *game_state)
{
	if (!action_runner->started) {
		action_runner->started = true;
		action_runner->paused = action_runner->completed = false;
		action_runner->start_time = game_state->time;
		// Invoke start function if available
		if (action_runner->action->start) {
			action_runner->action->start(action_runner, id, game_state);
		}
	}
	if (!action_runner->paused) {
		if (!action_runner->completed) {
			action_runner->action->update(action_runner, id, game_state);
		}
		// Check if action completed after the update
		if (action_runner->completed) {
			if (action_runner->action->loop) {
				action_runner->completed = false;
				action_runner->start_time = game_state->time;
			}
		}
	}
}

void action_stop(running_action_idx_t running_action_idx,
				 entity_id_t id,
				 struct game_state *game_state)
{
	entity_get(game_state->entity_repository, id)->action.running_actions[running_action_idx].action =
		NULL;
}
