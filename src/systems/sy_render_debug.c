#include "../assets/assets.h"
#include "../components/components.h"
#include "../entities/entities.h"
#include "../sprites/sprites.h"

void systems_render_debug(GameState *game_state) {
    EntityRepository *entity_repository = game_state->entity_repository;

    ComponentForEach(entity_repository, base_component, idx) {
        Component *base_sprite = base_component + CT_SPRITE * entity_repository->max_slots;
        Component *base_transform = base_component + CT_TRANSFORM * entity_repository->max_slots;
        if (entity_repository->entities[idx].alive && base_sprite->header.enabled &&
            base_transform->header.enabled) {
            ComponentSprite *sprite = &base_sprite->sprite;
            ComponentTransform *transform = &base_transform->transform;

            Vector2 position = transform->position;
            f32 rotation = transform->rotation;
            Vector2 scale = transform->scale;
            components_transform_calculate_absolute(transform, &position, &rotation, &scale);

            // TODO(marco): not great but I haven't found a better way to do it yet
            position.y -= transform->virtual_height;

            sprites_draw_debug_frame(
                assets_sprite_frame_get_by_id(game_state->sprite_frame_cache, sprite->frame_id),
                position, rotation, scale, sprite->pivot_override);
        }
    }
}
