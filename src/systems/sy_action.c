#include "entities.h"
#include "game.h"

void systems_action(struct game_state *game_state)
{
    FOR_EACH_ENTITY(entity, game_state->entity_repository)
    {
	if (!entities_is_entity_alive_and_has_components(entity, 1, C_ACTION)) {
	    continue;
	}

	DECLARE_ENTITY_COMPONENT(entity, action);

	for (usize j = 0; j < MaxConcurrentActions; ++j) {
	    struct action_runner *action_runner = &action->running_actions[j];
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
