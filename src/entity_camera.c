#include "entity_camera.h"
#include "entity.h"
#include "layer.h"
#include "log.h"
#include "physics.h"
#include "steering.h"
#include <raymath.h>
#include "game.h"

struct entity *entity_camera_create(struct game_state *game_state)
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

void entity_camera_follow_target(struct entity *camera_entity, struct game_state *game_state)
{
	struct component_transform *transform = &camera_entity->transform;
	struct component_physics *physics = &camera_entity->physics;
	struct component_camera *camera = &camera_entity->camera;
	struct component_target *target = &camera_entity->target;

	struct entity *target_entity = entity_get(game_state->entity_repository, target->entity_id);
	struct component_transform *target_transform = &target_entity->transform;
	struct component_physics3d *target_physics = &target_entity->physics3d;

	v2 steering = Vector2Zero();
	if (target_transform && target_physics) {
		// Look ahead along the pitch while the ball travels up or down it
		if (fabsf(target_physics->velocity.z) > 30) {
			if (target_physics->velocity.z < 0) {
				camera->offset.y -= 70 * game_state->dt;
			} else {
				camera->offset.y += 70 * game_state->dt;
			}
			camera->offset.y = Clamp(camera->offset.y, -50, 50);
		} else {
			camera->offset.y *= 0.997f;
		}

		layer_debug_queue_draw_text(game_state->layer_debug,
									TextFormat("Camera offset y: %.2f", camera->offset.y));
		steering = steering_arrive(transform->position,
								   physics->velocity,
								   physics->max_velocity,
								   Vector2Add(target_transform->position, camera->offset),
								   camera->slowing_distance);
	}
	steering = Vector2Scale(steering, physics->max_velocity * physics->max_force);

	// The camera integrates unclamped on purpose: its max velocity is a steering input,
	// not a speed limit.
	physics_apply_force(physics, steering, game_state->dt);
	physics_apply_velocity(transform, physics, game_state->dt);
}
