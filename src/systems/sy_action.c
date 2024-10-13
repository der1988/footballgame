#include "actions.h"
#include "components.h"
#include "entities.h"

void systems_action(struct game_state *game_state)
{
    for_each_entity(entity, game_state->entity_repository)
    {
	if (!entities_is_entity_alive_and_has_components(entity, 1, C_ACTION))
	{
	    continue;
	}

	declare_entity_component(entity, Action, action);

	for (usize j = 0; j < MaxConcurrentActions; ++j)
	{
	    struct ActionRunner *action_runner = &action->running_actions[j];
	    if (action_runner && action_runner->action)
	    {
		if (action_runner->completed)
		{
		    // If action is completed remove from running actions
		    // This needs to happen before the update, so we give a full update cycle
		    // to interested parties to check for action completion.
		    action->running_actions[j].action = NULL;
		}
		else
		{
		    actions_action_run(action_runner, entity_id, game_state);
		}
	    }
	}
    }
}
