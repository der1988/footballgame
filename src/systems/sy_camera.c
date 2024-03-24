#include "../assets/assets.h"
#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "../sprites/sprites.h"
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>

void systems_camera(GameState *game_state) {

    EntityRepository *entity_repository = game_state->entity_repository;

    EntityForEach(entity_repository, entity, idx) {
        Component *base_target =
            &entity_repository->components[idx] + CT_TARGET * entity_repository->max_slots;
        Component *base_camera_props =
            &entity_repository->components[idx] + CT_CAMERA_PROPS * entity_repository->max_slots;
        if (entity_repository->entities[idx].alive && base_target->header.enabled &&
            base_camera_props->header.enabled) {
            ComponentCameraProps *camera_props = &base_camera_props->camera_props;
            ComponentTarget *target = &base_target->target;

            if (target->target_transform) {
                game_state->camera.target = target->target_transform->position;
            }
        }
    }
}
