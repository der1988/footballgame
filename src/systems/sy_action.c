#include "../actions/actions.h"
#include "../components/components.h"
#include "../entities/entities.h"

void systems_action(GameState *game_state) {
    EntityRepository *entity_repository = game_state->entity_repository;

    ComponentForEach(entity_repository, base_component, idx) {
        Component *base_action = base_component + CT_ACTION * entity_repository->max_slots;
        Entity *entity = &entity_repository->entities[idx];
        if (entity->alive && base_action->header.enabled) {
            ComponentAction *component = &base_action->action;
            for (usize j = 0; j < MaxConcurrentActions; ++j) {
                ActionRunner *action_runner = &component->running_actions[j];
                if (action_runner && action_runner->action) {
                    if (action_runner->completed) {
                        // If action is completed remove from running actions
                        // This needs to happen before the update, so we give a full update cycle
                        // to interested parties to check for action completion.
                        component->running_actions[j].action = NULL;
                    } else {
                        actions_action_run(action_runner, entity->id, game_state);
                    }
                }
            }
        }
    }
}
