#include "components.h"
#include "entities.h"
#include "game.h"
#include "log.h"
#include <raylib.h>
#include <raymath.h>

struct Entity *entities_camera_create(struct GameState *game_state)
{
    struct Entity *entity =
	entities_entity_init(game_state->entity_repository, ET_CAMERA, "camera_%02d");

    // Initialise transform
    struct ComponentTransform *transform = &entity->transform;
    transform->header.enabled = true;
    transform->position = (Vector2){(f32)GetScreenWidth() / 2, -10};
    transform->rotation = 0;
    transform->scale = Vector2One();

    // Initialise physics
    struct ComponentPhysics *physics = &entity->physics;
    physics->header.enabled = true;
    physics->mass = 1;
    physics->velocity = (Vector2){0};
    physics->accel = (Vector2){0};
    physics->max_velocity = 2000.0f;
    // In case camera stutters or weird things happen this is the parameter to tweak
    physics->max_force = 50.0f;

    // Initialise props
    struct ComponentCamera *camera = &entity->camera;
    camera->header.enabled = true;
    camera->slowing_distance = 300.0f;

    // Initialise target
    struct ComponentTarget *target = &entity->target;
    target->header.enabled = true;

    GLOG_INFO_CTX(LS_ENTITIES, entity->header.name, "camera created");

    return entity;
}
