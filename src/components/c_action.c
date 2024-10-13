#include "actions.h"
#include "components.h"
#include "entities.h"
#include "game.h"
#include "types.h"
#include "log.h"
#include <assert.h>

running_action_idx_t components_action_run(entity_id_t id, struct action *action,
					   struct game_state *game_state)
{
    assert(action);

    struct entity *entity = entities_entity_get(game_state->entity_repository, id);

    assert(entity->action.header.enabled);
    struct action_runner *action_runner = NULL;

    running_action_idx_t idx = 0;
    for (idx = 0; idx < MaxConcurrentActions; ++idx)
    {
	action_runner = &entity->action.running_actions[idx];
	if (!action_runner->action)
	{
	    break;
	}
    }
    if (action_runner)
    {
	action_runner->action = action;
	action_runner->paused = action_runner->completed = action_runner->started = false;
    }
    else
    {
	GLOGF_WARN_CTX(LS_ENTITIES, entity->header.name,
		       "no slot available to run action of type %s: too many concurrent actions!",
		       ActionTypeToString[action->type]);
	return -1;
    }
    return idx;
}

void components_action_stop(entity_id_t id, running_action_idx_t running_action_idx,
			    struct game_state *game_state)
{
    struct entity *entity = entities_entity_get(game_state->entity_repository, id);

    entity->action.running_actions[running_action_idx].action = NULL;
}
