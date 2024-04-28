#include "../actions/actions.h"
#include "../components/components.h"
#include "../entities/entities.h"

void systems_action(GameState *game_state) {
    EntityRepository *entity_repository = game_state->entity_repository;

    for (usize entity_id = 0; entity_id < entity_repository->max_slots; ++entity_id) {
        if (entities_entity_is_alive_and_has_components(entity_repository, entity_id, 1,
                                                        CT_ACTION)) {
            ComponentAction *component =
                ComponentCastToTypedUnchecked(entity_repository, CT_ACTION, entity_id);
            for (usize j = 0; j < MaxConcurrentActions; ++j) {
                ActionRunner *action_runner = &component->running_actions[j];
                if (action_runner && action_runner->action) {
                    if (action_runner->completed) {
                        // If action is completed remove from running actions
                        // This needs to happen before the update, so we give a full update cycle
                        // to interested parties to check for action completion.
                        component->running_actions[j].action = NULL;
                    } else {
                        actions_action_run(action_runner, entity_id, game_state);
                    }
                }
            }
        }
    }
}
