#include "entity_repository.h"
#include "log.h"
#include <raymath.h>
#include "game.h"

struct entity *camera_create(struct game_state *game_state)
{
	struct entity *entity = entity_init(game_state->entity_repository, ET_CAMERA, "camera_%02d");

	// Initialise transform
	struct component_transform *transform = &entity->transform;
	transform->header.enabled = true;
	transform->position = (Vector2){(f32)GetScreenWidth() / 2, -10};
	transform->rotation = 0;
	transform->scale = Vector2One();

	// Initialise physics
	struct component_physics *physics = &entity->physics;
	physics->header.enabled = true;
	physics->mass = 1;
	physics->velocity = (Vector2){0};
	physics->accel = (Vector2){0};
	physics->max_velocity = 2000.0f;
	// In case camera stutters or weird things happen this is the parameter to tweak
	physics->max_force = 50.0f;

	// Initialise props
	struct component_camera *camera = &entity->camera;
	camera->header.enabled = true;
	camera->slowing_distance = 300.0f;

	// Initialise target
	struct component_target *target = &entity->target;
	target->header.enabled = true;

	GLOG_INFO_CTX(LS_ENTITIES, entity->header.name, "camera created");

	return entity;
}
