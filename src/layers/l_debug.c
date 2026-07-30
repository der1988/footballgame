#include "football/football.h"
#include "layers/layers_internal.h"
#include "log/log.h"
#include "game/game.h"
#include "entities/e_ball.h"
#include <assert.h>
#include <raylib.h>
#include <string.h>
#include <raymath.h>

#define MAX_FONTS 8

layer_debug_t *layer_debug_create(struct game_state *game_state)
{
	layer_debug_t *handle =
		memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(layer_debug_t));
	assert(handle);
	handle->initialised = true;

	handle->log_position = (v2){.x = 10, .y = 950};

	handle->frame_memory_arena = game_state->memory_by_lifetime[MAL_UPDATE];
	handle->color = WHITE;
	handle->font_size = 20;
	handle->spacing = 0;
	handle->space_between_rows = 16;
	handle->font = LoadFontEx("res/fonts/0xProtoNerdFontMono-Regular.ttf", 32, 0, 250);

	GLOG_INFO(LS_LAYERS, "layer debug initialised");

	return handle;
}

void layer_debug_destroy(layer_debug_t *handle)
{
	assert(handle->initialised);

	UnloadFont(handle->font);

	handle->initialised = false;

	GLOG_INFO(LS_LAYERS, "layer debug destroyed");
}

void layer_debug_reset(layer_debug_t *handle)
{
	assert(handle->initialised);
	handle->queued_requests = 0;
	GLOG_TRACE(LS_LAYERS, "layer debug reset");
}

void layer_debug_queue_draw_text(layer_debug_t *handle, const char *text)
{
	if (handle->queued_requests < MAX_QUEUED_DRAW_REQUESTS) {
		struct draw_text_request *dtr =
			memory_arena_push(handle->frame_memory_arena, sizeof(struct draw_text_request));
		assert(dtr);
		strncpy(dtr->text, text, ARRAY_LEN(dtr->text));
		handle->draw_text_requests[handle->queued_requests++] = dtr;
	} else {
		GLOG_WARN(LS_LAYERS, "max queued draw requests reached");
	}
}

void layer_debug_draw(layer_debug_t *handle)
{
	struct draw_text_request *dtr;
	for (usize i = 0; i < handle->queued_requests; ++i) {
		dtr = handle->draw_text_requests[i];
		DrawTextEx(handle->font,
				   dtr->text,
				   (v2){handle->log_position.x,
						handle->log_position.y + i * handle->space_between_rows},
				   handle->font_size,
				   handle->spacing,
				   handle->color);
	}
}
