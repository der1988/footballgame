#include "entities.h"
#include "game.h"
#include "steering.h"
#include <raymath.h>

void apply_force(Vector2 force_to_apply, struct component_physics *physics, delta_time_t dt)
{
	physics->accel = Vector2Scale(force_to_apply, 1 / physics->mass);

	// Calculate velocity
	physics->velocity = Vector2Add(physics->velocity, Vector2Scale(physics->accel, dt));
}

void apply_velocity(struct component_transform *transform,
					struct component_physics *physics,
					delta_time_t dt)
{
	transform->position = Vector2Add(transform->position, Vector2Scale(physics->velocity, dt));
}

void systems_camera(struct game_state *game_state)
{

	FOR_EACH_ENTITY(entity, game_state->entity_repository)
	{
		if (!entity_is_alive_and_has_components(
				entity, 4, C_TRANSFORM, C_PHYSICS, C_TARGET, C_CAMERA)) {
			continue;
		}

		DECLARE_ENTITY_COMPONENT(entity, transform);
		DECLARE_ENTITY_COMPONENT(entity, target);
		DECLARE_ENTITY_COMPONENT(entity, physics);
		DECLARE_ENTITY_COMPONENT(entity, camera);

		struct entity *target_entity = entity_get(game_state->entity_repository, target->entity_id);
		struct component_transform *target_transform = &target_entity->transform;
		struct component_physics3d *target_physics = &target_entity->physics3d;

		v2 steering = Vector2Zero();
		if (target_transform && target_physics) {
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

		apply_force(steering, physics, game_state->dt);
		apply_velocity(transform, physics, game_state->dt);
	}
}
