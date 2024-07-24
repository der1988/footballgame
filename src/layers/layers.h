#ifndef LAYERS_H
#define LAYERS_H

#include "../memory/memory.h"
#include "game.h"
#include "types.h"

#define MAX_QUEUED_DRAW_REQUESTS 50

typedef struct DrawTextRequest {
    char text[80];
} DrawTextRequest;

typedef struct LayersDebug {
    b32 initialised;
    MemoryArenaHandle *frame_memory_arena;
    u16 queued_requests;
    DrawTextRequest *draw_text_requests[MAX_QUEUED_DRAW_REQUESTS];
    Color color;
    u16 space_between_rows;
    u16 font_size;
} LayersDebug;

LayersDebug *layers_debug_create(GameState *game_state);
void layers_debug_destroy(LayersDebug *handle);
void layers_debug_reset(LayersDebug *handle);
void layers_debug_queue_draw_text(LayersDebug *handle, const char *text);
void layers_debug_draw(LayersDebug *handle);

void layers_hud_draw(GameState *game_state);
#endif // !LAYERS_H
