#include "../actions/actions.h"
#include "../assets/assets.h"
#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "../memory/memory.h"
#include "../utils/utils.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>

Entity *entities_camera_create(GameState *game_state) {
    Entity *entity = entities_entity_init(game_state->entity_repository, ET_CAMERA, "camera_%02d");

    // Initialise transform
    Component *component =
        entities_component_create(game_state->entity_repository, CT_TRANSFORM, entity->id);
    component->transform.position = (Vector2){(f32)GetScreenWidth() / 2, -10};
    component->transform.rotation = 0;
    component->transform.scale = Vector2One();

    // Initialise props
    entities_component_create(game_state->entity_repository, CT_CAMERA_PROPS, entity->id);

    // Initialise target
    entities_component_create(game_state->entity_repository, CT_TARGET, entity->id);

    GLOG_INFO_CTX(LS_ENTITIES, entity->name, "camera created");

    return entity;
}

void entities_camera_update(Entity *entity, GameState *game_state, Input *input) {
    ComponentTransform *transform =
        &entities_component_get(game_state->entity_repository, CT_TRANSFORM, entity->id)->transform;
}
