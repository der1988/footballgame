#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include <raylib.h>
#include <raymath.h>

void systems_render_hud(GameState *game_state) {
    EntityRepository *entity_repository = game_state->entity_repository;

    EntityId selected_player_entity_id = game_state->vars.p1.player_entity_id;
    ComponentTransform *transform = ComponentCastToTypedUnchecked(
        game_state->entity_repository, CT_TRANSFORM, selected_player_entity_id);
    DrawCircleV(Vector2Add(transform->position, (v2){.y = -25}), 2, RED);
}
