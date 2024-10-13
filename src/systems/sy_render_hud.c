#include "components.h"
#include "entities.h"
#include "game.h"
#include <raylib.h>
#include <raymath.h>

void systems_render_hud(struct game_state *game_state)
{
    entity_id_t selected_player_entity_id = game_state->vars.p1.player_entity_id;
    struct entity *entity =
	entities_entity_get(game_state->entity_repository, selected_player_entity_id);
    declare_entity_component(entity, transform);

    if (entity != NULL && entity->header.alive && transform->header.enabled)
    {
	DrawCircleV(Vector2Add(transform->position, (v2){.y = -25}), 2, RED);
    }
}
