#include "../actions/actions.h"
#include "../components/components.h"
#include "../entities/entities.h"
#include <raylib.h>

void actions_action_run(ActionRunner *action_runner, EntityId id, GameState *game_state) {
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
