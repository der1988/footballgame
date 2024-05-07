#include "../assets/assets.h"
#include "../components/components.h"
#include "../entities/entities.h"
#include "../sprites/sprites.h"
#include <raylib.h>
#include <raymath.h>

void systems_render_debug(GameState *game_state) {
    if (game_state->constants.render_debug) {
        EntityRepository *entity_repository = game_state->entity_repository;

        for (usize entity_id = 0; entity_id < entity_repository->max_slots; ++entity_id) {
            if (entities_entity_is_alive_and_has_components(entity_repository, entity_id, 2,
                                                            CT_TRANSFORM, CT_SPRITE)) {
                ComponentSprite *sprite =
                    ComponentCastToTypedUnchecked(entity_repository, CT_SPRITE, entity_id);
                ComponentTransform *transform =
                    ComponentCastToTypedUnchecked(entity_repository, CT_TRANSFORM, entity_id);
                Component *wrapper_collider =
                    entities_component_get(entity_repository, CT_COLLIDER, entity_id);

                Vector2 position = transform->position;
                f32 rotation = transform->rotation;
                Vector2 scale = transform->scale;
                components_transform_calculate_absolute(transform, &position, &rotation, &scale);

                // TODO(marco): not great but I haven't found a better way to do it yet
                position.y -= transform->virtual_height;

                sprites_draw_debug_frame(
                    assets_sprite_frame_get_by_id(game_state->sprite_frame_cache, sprite->frame_id),
                    position, rotation, scale, sprite->pivot_override);

                // Debug lines for pitch
                Rectangle pitch = game_state->vars.pitch_rectangle;
                DrawRectangleLines(pitch.x, pitch.y, pitch.width, pitch.height, BLUE);

                if (wrapper_collider->header.enabled) {
                    ComponentCollider *collider = &wrapper_collider->collider;
                    if (collider->shape == CS_CIRCLE) {
                        v2 pos = Vector2Add(transform->position, collider->props.circle.offset);
                        DrawCircleLinesV(pos, collider->props.circle.radius, RED);
                    } else if (collider->shape == CS_RECTANGLE) {
                        v2 pos = Vector2Add(transform->position, collider->props.rectangle.offset);
                        DrawRectangleLines(pos.x, pos.y, collider->props.rectangle.size.x,
                                           collider->props.rectangle.size.y, RED);
                    }
                }
            }
        }
    }
}
