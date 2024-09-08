#include "game.h"
#include "layers.h"
#include "memory.h"
#include "utils.h"
#include <assert.h>
#include <raylib.h>
#include <string.h>

LayersDebug *layers_debug_create(GameState *game_state) {
    LayersDebug *handle =
        memory_arena_push(game_state->memory_by_lifetime[MAL_GAME], sizeof(LayersDebug));
    handle->initialised = true;

    handle->frame_memory_arena = game_state->memory_by_lifetime[MAL_UPDATE];
    handle->color = WHITE;
    handle->font_size = 20;
    handle->space_between_rows = 20;

    GLOG_INFO(LS_LAYERS, "layer debug initialised");

    return handle;
}

void layers_debug_destroy(LayersDebug *handle) {
    assert(handle->initialised);
    handle->initialised = false;
    GLOG_INFO(LS_LAYERS, "layer debug destroyed");
}

void layers_debug_reset(LayersDebug *handle) {
    assert(handle->initialised);
    handle->queued_requests = 0;
    GLOG_TRACE(LS_LAYERS, "layer debug destroyed");
}

void layers_debug_queue_draw_text(LayersDebug *handle, const char *text) {
    if (handle->queued_requests < MAX_QUEUED_DRAW_REQUESTS) {
        DrawTextRequest *dtr =
            memory_arena_push(handle->frame_memory_arena, sizeof(DrawTextRequest));
        strncpy(dtr->text, text, ARRAY_LEN(dtr->text));
        handle->draw_text_requests[handle->queued_requests++] = dtr;
    } else {
        GLOG_WARN(LS_LAYERS, "max queued draw requests reached");
    }
}

void layers_debug_draw(LayersDebug *handle) {
    DrawTextRequest *dtr;
    for (usize i = 0; i < handle->queued_requests; ++i) {
        dtr = handle->draw_text_requests[i];
        DrawText(dtr->text, 50, 10 + i * handle->space_between_rows, handle->font_size,
                 handle->color);
    }
}
