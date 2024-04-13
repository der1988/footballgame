#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "../steering/steering.h"
#include <raylib.h>
#include <raymath.h>

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

    for (usize entity_id = 0; entity_id < entity_repository->max_slots; ++entity_id) {
        if (entities_entity_is_alive_and_has_components(entity_repository, entity_id, 4,
                                                        CT_TRANSFORM, CT_PHYSICS, CT_TARGET,
                                                        CT_CAMERA_PROPS)) {
            ComponentTransform *transform = ComponentCastToInternalUnchecked(
                game_state->entity_repository, CT_TRANSFORM, entity_id);
            ComponentPhysics *physics = ComponentCastToInternalUnchecked(
                game_state->entity_repository, CT_PHYSICS, entity_id);
            ComponentTarget *target = ComponentCastToInternalUnchecked(
                game_state->entity_repository, CT_TARGET, entity_id);
            ComponentCameraProps *props = ComponentCastToInternalUnchecked(
                game_state->entity_repository, CT_CAMERA_PROPS, entity_id);

            Component *target_transform = entities_component_get(game_state->entity_repository,
                                                                 CT_TRANSFORM, target->entity_id);
            v2 steering = Vector2Zero();
            if (target_transform && target_transform->header.enabled) {
                steering =
                    steering_arrive(transform->position, physics->velocity, physics->max_velocity,
                                    target_transform->transform.position, props->slowing_distance);
            }
            steering = Vector2Scale(steering, physics->max_velocity * physics->max_force);

            apply_force(steering, physics, game_state->dt);
            apply_velocity(transform, physics, game_state->dt);
            game_state->camera.target = transform->position;
        }
    }
}
