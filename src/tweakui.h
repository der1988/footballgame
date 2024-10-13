#ifndef TWEAKUI_H
#define TWEAKUI_H

#include "game.h"

typedef struct tweakui tweakui_t;

tweakui_t *tweakui_create(memory_arena_handle_t *memory_arena_handle);
void tweakui_destroy(tweakui_t *handle);
void tweakui_draw(tweakui_t *handle, struct game_state *game_state);

#endif // !TWEAKUI_H
