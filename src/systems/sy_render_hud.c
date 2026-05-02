#include "entities.h"
#include "game.h"
#include "constants.h"
#include "types.h"
#include <raymath.h>

static color_t g_player_colors[] = {RED, BLUE};

void systems_render_hud(struct game_state *game_state)
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
