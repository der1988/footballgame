#ifndef LAYERS_INTERNAL_H
#define LAYERS_INTERNAL_H

#include "memory.h"
#include "types.h"

#define MAX_QUEUED_DRAW_REQUESTS 50

struct draw_text_request
{
    char text[100];
};

struct layers_debug
{
    b32 initialised;
    memory_arena_handle_t *frame_memory_arena;
    u16 queued_requests;
    struct draw_text_request *draw_text_requests[MAX_QUEUED_DRAW_REQUESTS];
    color_t color;
    u16 spacing;
    u16 space_between_rows;
    font_t font;
    u16 font_size;
};

#endif
