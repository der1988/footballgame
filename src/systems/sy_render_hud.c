#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include <raylib.h>
#include <raymath.h>

void systems_render_hud(GameState *game_state) {
    EntityId selected_player_entity_id = game_state->vars.p1.player_entity_id;
    Entity *entity = entities_entity_get(game_state->entity_repository, selected_player_entity_id);
    if (entity != NULL && entity->header.alive && entity->transform.header.enabled) {
        ComponentTransform *transform = &entity->transform;
        DrawCircleV(Vector2Add(transform->position, (v2){.y = -25}), 2, RED);
    }
}
