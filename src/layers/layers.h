#ifndef LAYERS_H
#define LAYERS_H

#include "../memory/memory.h"
#include "game.h"
#include "types.h"

#define MAX_QUEUED_DRAW_REQUESTS 10

typedef struct DrawTextRequest {
    char text[40];
    f32 pos_x;
    f32 pos_y;
    Color color;
    u8 font_size;
} DrawTextRequest;

typedef struct LayersDebug {
    b32 initialised;
    MemoryArenaHandle *frame_memory_arena;
    u16 queued_requests;
    DrawTextRequest *draw_text_requests[MAX_QUEUED_DRAW_REQUESTS];
} LayersDebug;

LayersDebug *layers_debug_create(GameState *game_state);
void layers_debug_destroy(LayersDebug *handle);
void layers_debug_queue_draw_text(LayersDebug *handle, const char *text, f32 pos_x, f32 pos_y,
                                  u8 font_size, Color color);
void layers_debug_draw(LayersDebug *handle);
#endif // !LAYERS_H
