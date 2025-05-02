#ifndef ACTION_CHANGE_COLOR_H
#define ACTION_CHANGE_COLOR_H

#include "action.h"
#include "memory.h"

// Changes the sprite color from one color to another
struct action_change_color_from_to {
	struct action base;
	Color from;
	Color to;
};

struct action_change_color_from_to *actions_change_color_from_to_create(
	f32 duration, b32 loop, Color from, Color to, memory_arena_handle_t *memory_arena_handle);
void actions_change_color_from_to_update(struct action_runner *action_runner,
										 entity_id_t id,
										 struct game_state *game_state);

#endif // !ACTION_CHANGE_COLOR_H
