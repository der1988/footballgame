#include "layers_internal.h"
#include "game.h"
#include "memory.h"
#include "utils.h"
#include "log.h"
#include <assert.h>
#include <raylib.h>
#include <string.h>

#define MAX_FONTS 8

LayersDebug *layers_debug_create(struct GameState *game_state)
{
    LayersDebug *handle =
	memory_arena_push(game_state->memory_by_lifetime[MAL_GAME], sizeof(LayersDebug));
    handle->initialised = true;

    handle->frame_memory_arena = game_state->memory_by_lifetime[MAL_UPDATE];
    handle->color = WHITE;
    handle->font_size = 20;
    handle->spacing = 0;
    handle->space_between_rows = 16;
    handle->font = LoadFontEx("res/fonts/0xProtoNerdFontMono-Regular.ttf", 32, 0, 250);

    GLOG_INFO(LS_LAYERS, "layer debug initialised");

    return handle;
}

void layers_debug_destroy(LayersDebug *handle)
{
    assert(handle->initialised);

    UnloadFont(handle->font);

    handle->initialised = false;

    GLOG_INFO(LS_LAYERS, "layer debug destroyed");
}

void layers_debug_reset(LayersDebug *handle)
{
    assert(handle->initialised);
    handle->queued_requests = 0;
    GLOG_TRACE(LS_LAYERS, "layer debug destroyed");
}

void layers_debug_queue_draw_text(LayersDebug *handle, const char *text)
{
    if (handle->queued_requests < MAX_QUEUED_DRAW_REQUESTS)
    {
	struct DrawTextRequest *dtr =
	    memory_arena_push(handle->frame_memory_arena, sizeof(struct DrawTextRequest));
	strncpy(dtr->text, text, ARRAY_LEN(dtr->text));
	handle->draw_text_requests[handle->queued_requests++] = dtr;
    }
    else
    {
	GLOG_WARN(LS_LAYERS, "max queued draw requests reached");
    }
}

void layers_debug_draw(LayersDebug *handle)
{
    struct DrawTextRequest *dtr;
    for (usize i = 0; i < handle->queued_requests; ++i)
    {
	dtr = handle->draw_text_requests[i];
	DrawTextEx(handle->font, dtr->text, (v2){10, 10 + i * handle->space_between_rows},
		   handle->font_size, handle->spacing, handle->color);
    }
}
