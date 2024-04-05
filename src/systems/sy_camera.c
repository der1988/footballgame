#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "../steering/steering.h"
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

void apply_force(Vector2 force_to_apply, ComponentPhysics *physics, DeltaTime dt) {
    physics->accel = Vector2Scale(force_to_apply, 1 / physics->mass);

    // Calculate velocity
    physics->velocity = Vector2Add(physics->velocity, Vector2Scale(physics->accel, dt));
}

void apply_velocity(ComponentTransform *transform, ComponentPhysics *physics, DeltaTime dt) {
    transform->position = Vector2Add(transform->position, Vector2Scale(physics->velocity, dt));
}

void systems_camera(GameState *game_state) {

    EntityRepository *entity_repository = game_state->entity_repository;

    EntityForEach(entity_repository, entity, idx) {
        Component *base_transform =
            entities_component_get(game_state->entity_repository, CT_TRANSFORM, idx);
        Component *base_physics =
            entities_component_get(game_state->entity_repository, CT_PHYSICS, idx);
        Component *base_target =
            entities_component_get(game_state->entity_repository, CT_TARGET, idx);
        Component *base_camera_props =
            entities_component_get(game_state->entity_repository, CT_CAMERA_PROPS, idx);
        if (entity_repository->entities[idx].alive && base_transform->header.enabled &&
            base_physics->header.enabled && base_target->header.enabled &&
            base_camera_props->header.enabled) {
            ComponentTransform *transform = &base_transform->transform;
            ComponentPhysics *physics = &base_physics->physics;
            ComponentCameraProps *props = &base_camera_props->camera_props;
            ComponentTarget *target = &base_target->target;

            Component *target_transform = entities_component_get(game_state->entity_repository,
                                                                 CT_TRANSFORM, target->entity_id);
            v2 steering = Vector2Zero();
            if (target_transform && target_transform->header.enabled) {
                steering =
                    steering_arrive(transform->position, physics->velocity, physics->max_velocity,
                                    target_transform->transform.position, props->slowing_distance);
            }
            steering = Vector2Scale(steering, physics->max_force);

            apply_force(steering, physics, game_state->dt);
            apply_velocity(transform, physics, game_state->dt);
            game_state->camera.target = transform->position;
        }
    }
}
