#include "components.h"
#include "entities.h"
#include "game.h"
#include "layers.h"
#include "steering.h"
#include <raylib.h>
#include <raymath.h>

void apply_force(Vector2 force_to_apply, struct ComponentPhysics *physics, DeltaTime dt) {
    physics->accel = Vector2Scale(force_to_apply, 1 / physics->mass);

    // Calculate velocity
    physics->velocity = Vector2Add(physics->velocity, Vector2Scale(physics->accel, dt));
}

void apply_velocity(struct ComponentTransform *transform, struct ComponentPhysics *physics,
                    DeltaTime dt) {
    transform->position = Vector2Add(transform->position, Vector2Scale(physics->velocity, dt));
}

void systems_camera(struct GameState *game_state) {

    for_each_entity (entity, game_state->entity_repository) {
        struct ComponentTransform *transform = &entity->transform;
        struct ComponentTarget *target = &entity->target;
        struct ComponentPhysics *physics = &entity->physics;
        struct ComponentCamera *props = &entity->camera;
        if (entity->header.alive && transform->header.enabled && physics->header.enabled &&
            target->header.enabled && props->header.enabled) {

            struct Entity *target_entity =
                entities_entity_get(game_state->entity_repository, target->entity_id);
            struct ComponentTransform *target_transform = &target_entity->transform;
            struct ComponentPhysics3d *target_physics = &target_entity->physics_3d;

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
                steering = steering_arrive(transform->position,
                                           physics->velocity,
                                           physics->max_velocity,
                                           Vector2Add(target_transform->position, props->offset),
                                           props->slowing_distance);
            }
            steering = Vector2Scale(steering, physics->max_velocity * physics->max_force);

            apply_force(steering, physics, game_state->dt);
            apply_velocity(transform, physics, game_state->dt);
        }
    }
}
