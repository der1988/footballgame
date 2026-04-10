#ifndef VCRUI_H
#define VCRUI_H

#include "memory.h"

// Forward declaration
struct game_state;

typedef struct vcrui vcrui_t;

vcrui_t *vcrui_create(memory_arena_handle_t *memory_arena_handle);
void vcrui_destroy(vcrui_t *handle);
void vcrui_draw(vcrui_t *handle, struct game_state *game_state);

#endif // !VCRUI_H
