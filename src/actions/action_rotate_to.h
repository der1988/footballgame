#ifndef ACTION_ROTATE_TO_H
#define ACTION_ROTATE_TO_H

#include "action.h"
#include "memory.h"

// Rotate a float to an angle
struct action_rotate_to {
	struct action base;
	f32 from;
	f32 to;
	f32 diff;
};

struct action_rotate_to *actions_rotate_to_create(f32 duration,
												  b32 loop,
												  f32 to,
												  memory_arena_handle_t *memory_arena_handle);
void actions_rotate_to_update(struct action_runner *action_runner,
							  entity_id_t id,
							  struct game_state *game_state);

#endif // !ACTION_ROTATE_TO_H
