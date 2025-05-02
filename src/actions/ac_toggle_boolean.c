#include "ac_toggle_boolean.h"
#include "macro.h"

void action_toggle_boolean_update(struct action_runner *action_runner,
								  entity_id_t id,
								  struct game_state *game_state)
{
	UNUSED(id);
	UNUSED(game_state);

	struct action_toggle_boolean *action = (struct action_toggle_boolean *)action_runner->action;

	action_runner->completed = true;
	*action->value = !*action->value;
}

struct action_toggle_boolean *action_toggle_boolean_create(
	f32 duration, b32 loop, b32 *value, memory_arena_handle_t *memory_arena_handle)
{
	struct action_toggle_boolean *action = memory_arena_push(memory_arena_handle, sizeof(*action));
	*action = (struct action_toggle_boolean){
		.base = (struct action){.type = AT_TOGGLE_BOOLEAN,
								.duration = duration,
								.loop = loop,
								.update = action_toggle_boolean_update},
		.value = value,
	};
	return action;
}
