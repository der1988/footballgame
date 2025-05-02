#include "ac_tween_float_from_to.h"
#include "game.h"

void action_tween_float_from_to_update(struct action_runner *action_runner,
									   entity_id_t id,
									   struct game_state *game_state)
{
	UNUSED(id);

	struct action_tween_float_from_to *action =
		(struct action_tween_float_from_to *)action_runner->action;
	f32 h = (game_state->time - action_runner->start_time) / action_runner->action->duration;
	if (h >= 1) {
		h = 1;
		action_runner->completed = true;
	}

	*action->value = action->from + (action->to - action->from) * h;
}

struct action_tween_float_from_to *action_tween_float_from_to_create(
	f32 duration, b32 loop, f32 *value, f32 from, f32 to, memory_arena_handle_t *memory_arena_handle)
{
	struct action_tween_float_from_to *action =
		memory_arena_push(memory_arena_handle, sizeof(*action));
	*action = (struct action_tween_float_from_to){
		.base = (struct action){.type = AT_TWEEN_FLOAT_FROM_TO,
								.duration = duration,
								.loop = loop,
								.update = action_tween_float_from_to_update},
		.value = value,
		.from = from,
		.to = to,
	};
	return action;
}
