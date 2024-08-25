#include "../assets/assets.h"
#include "../components/components.h"
#include "../entities/entities.h"
#include "../sprites/sprites.h"
#include <raylib.h>
#include <raymath.h>

void systems_render_debug(GameState *game_state) {
    if (game_state->constants.render_debug) {
        EntityRepository *entity_repository = game_state->entity_repository;

        Entity *entity;
        for (usize entity_id = 0; entity_id < entity_repository->max_slots; ++entity_id) {
            entity = entities_entity_get(game_state->entity_repository, entity_id);
            if (entity->header.alive && entity->transform.header.enabled &&
                entity->sprite.header.enabled) {
                ComponentTransform *transform = &entity->transform;
                ComponentSprite *sprite = &entity->sprite;
                ComponentCollider *collider = &entity->collider;

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
                Rectangle pitch = game_state->vars.pitch.rectangle;
                DrawRectangleLines(pitch.x, pitch.y, pitch.width, pitch.height, BLUE);

                if (collider->header.enabled) {
                    static Color colliders_colors[] = {RED, VIOLET, PINK, YELLOW};
                    for (u16 i = 0; i < collider->num_colliders; ++i) {
                        if (collider->colliders[i].shape == CS_CIRCLE) {
                            v2 pos = Vector2Add(transform->position,
                                                collider->colliders[i].props_circle.offset);
                            DrawCircleLinesV(pos, collider->colliders[i].props_circle.radius,
                                             colliders_colors[i]);
                        } else if (collider->colliders[i].shape == CS_RECTANGLE) {
                            v2 pos = Vector2Add(transform->position,
                                                collider->colliders[i].props_rectangle.offset);
                            DrawRectangleLines(
                                pos.x, pos.y, collider->colliders[i].props_rectangle.size.x,
                                collider->colliders[i].props_rectangle.size.y, colliders_colors[i]);
                        }
                    }
                }
            }
        }

        // Ball debug
        ComponentBall *ball =
            &entities_entity_get(game_state->entity_repository, game_state->vars.ball_entity_id)
                 ->ball;
        for (u8 i = 0; i < 25; ++i) {
            DrawCircle(ball->lookahead_positions[i].x, ball->lookahead_positions[i].z, 1, BLUE);
        }
        DrawCircle(ball->future_bounce_position.x, ball->future_bounce_position.y, 1, PURPLE);
    }
}
