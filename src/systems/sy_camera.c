#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "../steering/steering.h"
#include "layers.h"
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

    Entity *entity;
    for (usize entity_id = 0; entity_id < entity_repository->max_slots; ++entity_id) {
        entity = entities_entity_get(game_state->entity_repository, entity_id);
        if (entity->header.alive && entity->transform.header.enabled &&
            entity->physics.header.enabled && entity->target.header.enabled &&
            entity->camera.header.enabled) {
            ComponentTransform *transform = &entity->transform;
            ComponentTarget *target = &entity->target;
            ComponentPhysics *physics = &entity->physics;
            ComponentCamera *props = &entity->camera;

            Entity *target_entity =
                entities_entity_get(game_state->entity_repository, target->entity_id);
            ComponentTransform *target_transform = &target_entity->transform;
            ComponentPhysics3d *target_physics = &target_entity->physics_3d;

            v2 steering = Vector2Zero();
            if (target_transform && target_physics) {
                if (fabsf(target_physics->velocity.z) > 30) {
                    if (target_physics->velocity.z < 0) {
                        props->offset.y -= 70 * game_state->dt;
                    } else {
                        props->offset.y += 70 * game_state->dt;
                    }
                    props->offset.y = Clamp(props->offset.y, -50, 50);
                } else {
                    props->offset.y *= 0.997f;
                }

                layers_debug_queue_draw_text(game_state->layer_debug,
                                             TextFormat("Camera offset y: %.2f", props->offset.y));
                steering = steering_arrive(
                    transform->position, physics->velocity, physics->max_velocity,
                    Vector2Add(target_transform->position, props->offset), props->slowing_distance);
            }
            steering = Vector2Scale(steering, physics->max_velocity * physics->max_force);

            apply_force(steering, physics, game_state->dt);
            apply_velocity(transform, physics, game_state->dt);
        }
    }
}
