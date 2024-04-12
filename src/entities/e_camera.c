#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "../utils/utils.h"
#include <raylib.h>
#include <raymath.h>

Entity *entities_camera_create(GameState *game_state) {
    Entity *entity = entities_entity_init(game_state->entity_repository, ET_CAMERA, "camera_%02d");

    // Initialise transform
    Component *component =
        entities_component_create(game_state->entity_repository, CT_TRANSFORM, entity->id);
    component->transform.position = (Vector2){(f32)GetScreenWidth() / 2, -10};
    component->transform.rotation = 0;
    component->transform.scale = Vector2One();

    // Initialise physics
    component = entities_component_create(game_state->entity_repository, CT_PHYSICS, entity->id);
    component->physics.mass = 1;
    component->physics.velocity = (Vector2){0};
    component->physics.accel = (Vector2){0};
    component->physics.max_velocity = 2000.0f;
    // In case camera stutters or weird things happen this is the parameter to tweak
    component->physics.max_force = 50.0f;

    // Initialise props
    component =
        entities_component_create(game_state->entity_repository, CT_CAMERA_PROPS, entity->id);
    component->camera_props.slowing_distance = 300.0f;

    // Initialise target
    entities_component_create(game_state->entity_repository, CT_TARGET, entity->id);

    GLOG_INFO_CTX(LS_ENTITIES, entity->name, "camera created");

    return entity;
}
