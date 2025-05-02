#include "football.h"
#include "layers_internal.h"
#include "log.h"
#include "game.h"
#include "entities.h"
#include <assert.h>
#include <raylib.h>
#include <string.h>
#include <raymath.h>

#define MAX_FONTS 8

layers_debug_t *layers_debug_create(struct game_state *game_state)
{
	layers_debug_t *handle =
		memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(layers_debug_t));
	handle->initialised = true;

	handle->log_position = (v2){.x = 10, .y = 600};
	handle->radar_position = (v2){.x = 10, .y = 10};

	handle->frame_memory_arena = game_state->memory_by_lifetime[MAL_UPDATE];
	handle->color = WHITE;
	handle->font_size = 20;
	handle->spacing = 0;
	handle->space_between_rows = 16;
	handle->font = LoadFontEx("res/fonts/0xProtoNerdFontMono-Regular.ttf", 32, 0, 250);

	GLOG_INFO(LS_LAYERS, "layer debug initialised");

	return handle;
}

void layers_debug_destroy(layers_debug_t *handle)
{
	assert(handle->initialised);

	UnloadFont(handle->font);

	handle->initialised = false;

	GLOG_INFO(LS_LAYERS, "layer debug destroyed");
}

void layers_debug_reset(layers_debug_t *handle)
{
	assert(handle->initialised);
	handle->queued_requests = 0;
	GLOG_TRACE(LS_LAYERS, "layer debug destroyed");
}

void layers_debug_queue_draw_text(layers_debug_t *handle, const char *text)
{
	if (handle->queued_requests < MAX_QUEUED_DRAW_REQUESTS) {
		struct draw_text_request *dtr =
			memory_arena_push(handle->frame_memory_arena, sizeof(struct draw_text_request));
		strncpy(dtr->text, text, ARRAY_LEN(dtr->text));
		handle->draw_text_requests[handle->queued_requests++] = dtr;
	} else {
		GLOG_WARN(LS_LAYERS, "max queued draw requests reached");
	}
}

static inline v2 to_radar_position(v2 pos, rect_t pitch_rect, v2 radar_pos, f32 ratio)
{
	v2 radar_relative_pos = Vector2Subtract(pos, (v2){.x = pitch_rect.x, .y = pitch_rect.y});
	radar_relative_pos = Vector2Scale(radar_relative_pos, ratio);
	return Vector2Add(radar_relative_pos, radar_pos);
}

void layers_debug_draw(layers_debug_t *handle, struct game_state *game_state)
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

	// Radar
	f32 radar_ratio = 0.3f;
	rect_t radar_rect = (rect_t){.width = game_state->vars.pitch.field.width * radar_ratio,
								 .height = game_state->vars.pitch.field.height * radar_ratio,
								 .x = handle->radar_position.x,
								 .y = handle->radar_position.y};

	DrawRectangleLinesEx(radar_rect, 1.0f, WHITE);

	struct entity *ball_entity = game_state->vars.ball_entity;

	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
	UNUSED(ball_physics_3d);
	UNUSED(ball);
	UNUSED(ball_collider);

	DrawCircleV(to_radar_position(ball_transform->position,
								  game_state->vars.pitch.field,
								  handle->radar_position,
								  radar_ratio),
				3,
				WHITE);

	FOR_EACH_ENTITY(entity, game_state->entity_repository)
	{
		if (!entity_is_alive_and_has_components(
				entity, 5, C_TRANSFORM, C_PHYSICS, C_SPRITE, C_PLAYER, C_COLLIDER)) {
			continue;
		}

		DECLARE_ENTITY_COMPONENT(entity, transform);

		DrawRectangleV(to_radar_position(transform->position,
										 game_state->vars.pitch.field,
										 handle->radar_position,
										 radar_ratio),
					   (v2){.x = 6, .y = 4},
					   entity->player.team_side == TS_HOME ? RED : BLUE);
	}
}
