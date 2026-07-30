#ifndef ACTION_SEQUENCE_H
#define ACTION_SEQUENCE_H

#include "actions/action.h"
#include "memory/memory.h"

// Concatenates multiple actions to run them in series
struct action_sequence {
	struct action base;
	u16 count;
	struct action_runner actions[];
};

struct action_sequence *action_sequence_create(memory_arena_handle_t *memory_arena_handle,
											   u32 count,
											   ...);
void action_sequence_update(struct action_runner *action_runner,
							entity_id_t id,
							struct game_state *game_state);

#endif // !ACTION_SEQUENCE_H
