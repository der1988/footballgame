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
	if (game_state->gameplay.restart.prepared) {
		if (game_state->gameplay.restart.manual) {
			const v2 spot = game_state->gameplay.restart.spot;
			const v2 end = Vector2Add(spot, Vector2Scale(game_state->gameplay.restart.aim, 55));
			DrawLineEx(spot, end, 3, YELLOW);
			DrawCircleV(end, 4, YELLOW);
			DrawRectangle(spot.x - 30, spot.y - 45, 60, 7, DARKGRAY);
			DrawRectangle(spot.x - 30, spot.y - 45, 60 * game_state->gameplay.restart.power, 7, GREEN);
			DrawText("Aim + hold/release A/S/D", spot.x - 90, spot.y - 85, 12, WHITE);
		}
		const char *label = state_machine_football_state_string[game_state->gameplay.state_machine.current_state];
		DrawText(label, game_state->vars.ball.entity->transform.position.x - 50,
			game_state->vars.ball.entity->transform.position.y - 65, 18, YELLOW);
	}
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
