#include "actions.h"
#include <raylib.h>

void actions_toggle_boolean_update(struct ActionRunner *action_runner, entity_id_t id,
				   struct game_state *game_state)
{
    struct ActionToggleBoolean *action = (struct ActionToggleBoolean *)action_runner->action;

    action_runner->completed = true;
    *action->value = !*action->value;
}

struct ActionToggleBoolean *
actions_toggle_boolean_create(f32 duration, b32 loop, b32 *value,
			      memory_arena_handle_t *memory_arena_handle)
{
    struct ActionToggleBoolean *action = memory_arena_push(memory_arena_handle, sizeof(*action));
    *action = (struct ActionToggleBoolean){
	.base = (struct Action){.type = AT_TOGGLE_BOOLEAN,
				.duration = duration,
				.loop = loop,
				.update = actions_toggle_boolean_update},
	.value = value,
    };
    return action;
}
