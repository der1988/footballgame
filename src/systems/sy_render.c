#include "../assets/assets.h"
#include "../components/components.h"
#include "../entities/entities.h"
#include "../game.h"
#include "../sprites/sprites.h"
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>

typedef struct {
    AssetsSpriteFrame *sprite_frame;
    AssetsTexture *texture;
    SpriteLayer layer;
    i16 order_in_layer;
    Color color;
    Vector2 position;
    f32 rotation;
    Vector2 scale;
    b32 flipX;
    b32 flipY;
} SpriteToDraw;

static i32 cmp_sprite_order(const void *s1, const void *s2) {
    const SpriteToDraw *sprite_1 = (SpriteToDraw *)s1;
    const SpriteToDraw *sprite_2 = (SpriteToDraw *)s2;

    i32 diff = 0;
    // First layer, then order in layer
    if ((diff = sprite_1->layer - sprite_2->layer) == 0) {
        diff = sprite_1->order_in_layer - sprite_2->order_in_layer;
    }
    return diff;
}

void systems_render(GameState *game_state) {
    EntityRepository *entity_repository = game_state->entity_repository;

    // Allocate some buffer in frame memory arena
    SpriteToDraw *ordered_sprites_to_draw =
        memory_arena_push(game_state->memory_by_lifetime[MAL_FRAME],
                          sizeof(SpriteToDraw) * entity_repository->max_slots);

    // Collect information about sprites to draw
    SpriteToDraw *current_idx = ordered_sprites_to_draw;
    ComponentForEach(entity_repository, base_component, idx) {
        Component *base_sprite = base_component + CT_SPRITE * entity_repository->max_slots;
        Component *base_transform = base_component + CT_TRANSFORM * entity_repository->max_slots;
        if (entity_repository->entities[idx].alive && base_sprite->header.enabled &&
            base_transform->header.enabled) {
            ComponentSprite *sprite = &base_sprite->sprite;
            ComponentTransform *transform = &base_transform->transform;
            AssetsSpriteFrame *sprite_frame =
                assets_sprite_frame_get_by_id(game_state->sprite_frame_cache, sprite->frame_id);

            Vector2 position = transform->position;
            f32 rotation = transform->rotation;
            Vector2 scale = transform->scale;
            components_transform_calculate_absolute(transform, &position, &rotation, &scale);
            AssetsTexture *texture =
                assets_texture_get_by_id(game_state->texture_cache, sprite_frame->texture_id);

            *current_idx = (SpriteToDraw){.sprite_frame = sprite_frame,
                                          .layer = sprite->layer,
                                          .order_in_layer = sprite->order_in_layer,
                                          .position = position,
                                          .rotation = rotation,
                                          .scale = scale,
                                          .color = sprite->color,
                                          .flipX = sprite->flipX,
                                          .flipY = sprite->flipY,
                                          .texture = texture};
            current_idx++;
        }
    }
    u32 total_sprites = current_idx - ordered_sprites_to_draw;

    qsort(ordered_sprites_to_draw, total_sprites, sizeof(SpriteToDraw), cmp_sprite_order);

    for (usize i = 0; i < total_sprites; ++i) {
        SpriteToDraw *sprite_to_draw = &ordered_sprites_to_draw[i];
        sprites_draw(sprite_to_draw->sprite_frame, sprite_to_draw->position,
                     sprite_to_draw->rotation, sprite_to_draw->scale, sprite_to_draw->color,
                     sprite_to_draw->flipX, sprite_to_draw->flipY, sprite_to_draw->texture);
    }
}
