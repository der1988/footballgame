#include "../actions/actions.h"
#include "../components/components.h"
#include "../entities/entities.h"
#include "../utils/utils.h"
#include "game.h"
#include "types.h"
#include <assert.h>

RunningActionIdx components_action_run(EntityId id, struct Action *action, GameState *game_state) {
    assert(action);

    Entity *entity = entities_entity_get(game_state->entity_repository, id);

    assert(entity->action.header.enabled);
    ActionRunner *action_runner = NULL;

    RunningActionIdx idx = 0;
    for (idx = 0; idx < MaxConcurrentActions; ++idx) {
        action_runner = &entity->action.running_actions[idx];
        if (!action_runner->action) {
            break;
        }
    }
    if (action_runner) {
        action_runner->action = action;
        action_runner->paused = action_runner->completed = action_runner->started = false;
    } else {
        GLOGF_WARN_CTX(LS_ENTITIES, entity->header.name,
                       "no slot available to run action of type %s: too many concurrent actions!",
                       ActionTypeToString[action->type]);
        return -1;
    }
    return idx;
}

void components_action_stop(EntityId id, RunningActionIdx running_action_idx,
                            GameState *game_state) {
    Entity *entity = entities_entity_get(game_state->entity_repository, id);

    entity->action.running_actions[running_action_idx].action = NULL;
}
