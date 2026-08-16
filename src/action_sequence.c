#include "action_sequence.h"
#include "macro.h"
#include <stdarg.h>

static void action_sequence_start(struct action_runner *action_runner,
						   entity_id_t id,
						   struct game_state *game_state)
{
	UNUSED(id);
	UNUSED(game_state);

	struct action_sequence *action_sequence = (struct action_sequence *)action_runner->action;

	// Reset all children actions
	for (usize i = 0; i < action_sequence->count; ++i) {
		action_sequence->actions[i].started = false;
	}
}

void action_sequence_update(struct action_runner *action_runner,
							entity_id_t id,
							struct game_state *game_state)
{
	struct action_sequence *action_sequence = (struct action_sequence *)action_runner->action;

	usize i = 0;
	for (i = 0; i < action_sequence->count; ++i) {
		action_run(&action_sequence->actions[i], id, game_state);
		if (!action_sequence->actions[i].completed) {
			break;
		}
	}

	if (i == action_sequence->count) {
		action_runner->completed = true;
	}
}

struct action_sequence *action_sequence_create(memory_arena_handle_t *memory_arena_handle,
											   u32 count,
											   ...)
{
	struct action_sequence *action_sequence = memory_arena_push(
		memory_arena_handle, sizeof(struct action_sequence) + sizeof(struct action_runner) * count);
	ASSERT(action_sequence);
	*action_sequence = (struct action_sequence){
		.base = (struct action){.type = AT_SEQUENCE,
								.loop = false,
								.start = action_sequence_start,
								.update = action_sequence_update},
	};

	f32 total_duration = 0;
	va_list ap;
	va_start(ap, count);
	for (usize i = 0; i < count; ++i) {
		action_sequence->actions[i] = (struct action_runner){.action = va_arg(ap, struct action *)};
		total_duration += action_sequence->actions[i].action->duration;
	}
	va_end(ap);

	action_sequence->count = count;
	action_sequence->base.duration = total_duration;

	return action_sequence;
}
