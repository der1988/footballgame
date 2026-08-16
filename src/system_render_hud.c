#include "macro.h"
#include "system.h"
#include "entity.h"
#include "game.h"
#include "constants.h"
#include "math_ext.h"
#include "types.h"
#include <raymath.h>

static color_t g_player_colors[] = {RED, BLUE};

void system_render_hud_front(struct game_state *game_state)
{
	for (usize i = 0; i < MAX_HUMAN_PLAYERS; ++i) {
		if (!game_state->human_players[i].enabled)
			continue;
		entity_id_t selected_player_entity_id =
			game_state->human_players[i].controlled_player.entity_id;
		struct entity *entity =
			entity_get(game_state->entity_repository, selected_player_entity_id);
		DECLARE_ENTITY_COMPONENT(entity, transform);

		if (entity != NULL && entity->header.alive && transform->header.enabled) {
			DrawCircleV(Vector2Add(transform->position, (v2){.y = -25}), 2, g_player_colors[i]);
		}
	}
}

void system_render_hud_back(struct game_state *game_state)
{
	// Next bounce crosshair
	struct future_position *bounce_position =
		&game_state->vars.ball.entity->ball.future_positions[FUTURE_POSITION_BOUNCE];
	if (game_state->vars.hud.show_crosshair && bounce_position->available) {
		struct asset_sprite_frame *sprite_frame = asset_sprite_frame_get_by_id(
			game_state->assets.sprite_frame_cache, game_state->gfx.hud.crosshair);
		ASSERT(sprite_frame);

		struct asset_texture *texture =
			asset_texture_get_by_id(game_state->assets.texture_cache, sprite_frame->texture_id);
		ASSERT(texture);

		render_sprite_draw(sprite_frame,
						   V3_TO_V2(bounce_position->position),
						   0,
						   Vector2One(),
						   WHITE,
						   false,
						   false,
						   (v2){.x = 0.5f, .y = 0.5f},
						   texture);
	}
}
