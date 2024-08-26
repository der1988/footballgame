#include "../actions/actions.h"
#include "../components/components.h"
#include "../entities/entities.h"

void systems_action(GameState *game_state) {
    EntityRepository *entity_repository = game_state->entity_repository;

    Entity *entity;
    for (usize entity_id = 0; entity_id < entity_repository->max_slots; ++entity_id) {
        entity = entities_entity_get(game_state->entity_repository, entity_id);
        ComponentAction *action = &entity->action;
        if (entity->header.alive && action->header.enabled) {
            for (usize j = 0; j < MaxConcurrentActions; ++j) {
                ActionRunner *action_runner = &action->running_actions[j];
                if (action_runner && action_runner->action) {
                    if (action_runner->completed) {
                        // If action is completed remove from running actions
                        // This needs to happen before the update, so we give a full update cycle
                        // to interested parties to check for action completion.
                        action->running_actions[j].action = NULL;
                    } else {
                        actions_action_run(action_runner, entity_id, game_state);
                    }
                }
            }
        }
    }
}
