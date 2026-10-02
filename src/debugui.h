#ifndef DEBUGUI_H
#define DEBUGUI_H

#include "memory.h"

// Forward declaration
struct game_state;

typedef struct debugui debugui_t;

debugui_t *debugui_create(memory_arena_handle_t *memory_arena_handle);
void debugui_destroy(debugui_t *handle);
void debugui_vcr_draw(debugui_t *handle, struct game_state *game_state);
void debugui_entity_inspector_draw(debugui_t *handle, struct game_state *game_state);
void debugui_constants_inspector_draw(debugui_t *handle, struct game_state *game_state);

// Returns true (and clears the flag) if a constant was edited since the last call. Call at the
// point control returns to the game to decide whether to publish EVT_CONSTANTS_CHANGED once.
b32 debugui_consume_constants_dirty(debugui_t *handle);
void debugui_debug_flags_draw(debugui_t *handle, struct game_state *game_state);
void debugui_game_state_draw(debugui_t *handle, struct game_state *game_state);

#endif // !TWEAKUI_H
