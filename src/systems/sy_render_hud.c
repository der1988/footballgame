#include "components.h"
#include "entities.h"
#include "game.h"
#include <raylib.h>
#include <raymath.h>

void systems_render_hud(struct GameState *game_state)
{
    EntityId selected_player_entity_id = game_state->vars.p1.player_entity_id;
    struct Entity *entity =
	entities_entity_get(game_state->entity_repository, selected_player_entity_id);
    declare_entity_component(entity, Transform, transform);

    if (entity != NULL && entity->header.alive && transform->header.enabled)
    {
	DrawCircleV(Vector2Add(transform->position, (v2){.y = -25}), 2, RED);
    }
}
