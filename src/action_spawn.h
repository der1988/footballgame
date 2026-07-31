#ifndef ACTION_SPAWN_H
#define ACTION_SPAWN_H

#include "action.h"
#include "memory.h"

// Spawn multiple actions to run them in parallel
struct action_spawn {
	struct action base;
	u16 count;
	struct action_runner actions[];
};

struct action_spawn *action_spawn_create(memory_arena_handle_t *memory_arena_handle,
										 u32 count,
										 ...);
void action_spawn_update(struct action_runner *action_runner,
						 entity_id_t id,
						 struct game_state *game_state);

#endif // !ACTION_SEQUENCE_H
