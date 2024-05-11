#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "../utils/utils.h"
#include <raylib.h>
#include <raymath.h>

Entity *entities_camera_create(GameState *game_state) {
    Entity *entity = entities_entity_init(game_state->entity_repository, ET_CAMERA, "camera_%02d");

    // Initialise transform
    ComponentTransform *transform =
        entities_component_create(game_state->entity_repository, CT_TRANSFORM, entity->id);
    transform->position = (Vector2){(f32)GetScreenWidth() / 2, -10};
    transform->rotation = 0;
    transform->scale = Vector2One();

    // Initialise physics
    ComponentPhysics *physics =
        entities_component_create(game_state->entity_repository, CT_PHYSICS, entity->id);
    physics->mass = 1;
    physics->velocity = (Vector2){0};
    physics->accel = (Vector2){0};
    physics->max_velocity = 2000.0f;
    // In case camera stutters or weird things happen this is the parameter to tweak
    physics->max_force = 50.0f;

    // Initialise props
    ComponentCamera *camera =
        entities_component_create(game_state->entity_repository, CT_CAMERA, entity->id);
    camera->slowing_distance = 300.0f;

    // Initialise target
    entities_component_create(game_state->entity_repository, CT_TARGET, entity->id);

    GLOG_INFO_CTX(LS_ENTITIES, entity->name, "camera created");

    return entity;
}
