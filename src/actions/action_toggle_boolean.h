#ifndef ACTION_TOGGLE_BOOLEAN_H
#define ACTION_TOGGLE_BOOLEAN_H

#include "action.h"
#include "memory.h"

// Toggle boolean
struct action_toggle_boolean {
	struct action base;
	b32 *value;
};

void action_toggle_boolean_update(struct action_runner *action_runner,
								  entity_id_t id,
								  struct game_state *game_state);
struct action_toggle_boolean *action_toggle_boolean_create(
	f32 duration, b32 loop, b32 *value, memory_arena_handle_t *memory_arena_handle);

#endif // !ACTION_TOGGLE_BOOLEAN_H
