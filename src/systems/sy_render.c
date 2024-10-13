#include "assets.h"
#include "components.h"
#include "entities.h"
#include "game.h"
#include "render.h"
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>

struct SpriteToDraw
{
    struct AssetsSpriteFrame *sprite_frame;
    struct AssetsTexture *texture;
    enum sprite_layer layer;
    i16 order_in_layer;
    Color color;
    Vector2 position;
    f32 height;
    f32 rotation;
    Vector2 scale;
    b32 flipX;
    b32 flipY;
    v2 pivot_override;
    b32 draw_shadow;
    v2 shadow_offset;
    v2 shadow_scale;
    Color shadow_color;
};

static i32 cmp_sprite_order(const void *s1, const void *s2)
{
    const struct SpriteToDraw *sprite_1 = (struct SpriteToDraw *)s1;
    const struct SpriteToDraw *sprite_2 = (struct SpriteToDraw *)s2;

    i32 diff = 0;
    // First layer, then order in layer
    if ((diff = sprite_1->layer - sprite_2->layer) == 0)
    {
	diff = sprite_1->order_in_layer - sprite_2->order_in_layer;
	if (diff == 0)
	{
	    // Order by y position: on the pitch a sprite positioned lower is in front of a
	    // sprite positioned higher.
	    // Multiply by 100 to preserve precision
	    diff = sprite_1->position.y * 100 - sprite_2->position.y * 100;
	}
    }
    return diff;
}

void systems_render(struct GameState *game_state)
{
    EntityRepository *entity_repository = game_state->entity_repository;

    // Allocate some buffer in frame memory arena
    struct SpriteToDraw *ordered_sprites_to_draw =
	memory_arena_push(game_state->memory_by_lifetime[MAL_DRAW],
			  sizeof(struct SpriteToDraw) * entity_repository->max_slots);

    // Collect information about sprites to draw
    struct SpriteToDraw *current_idx = ordered_sprites_to_draw;
    for_each_entity(entity, game_state->entity_repository)
    {

	if (!entities_is_entity_alive_and_has_components(entity, 2, C_TRANSFORM, C_SPRITE))
	{
	    continue;
	}

	declare_entity_component(entity, Transform, transform);
	declare_entity_component(entity, Sprite, sprite);

	struct AssetsSpriteFrame *sprite_frame =
	    assets_sprite_frame_get_by_id(game_state->sprite_frame_cache, sprite->frame_id);

	Vector2 position = transform->position;
	f32 rotation = transform->rotation;
	Vector2 scale = transform->scale;
	components_transform_calculate_absolute(transform, &position, &rotation, &scale);
	struct AssetsTexture *texture =
	    assets_texture_get_by_id(game_state->texture_cache, sprite_frame->texture_id);

	// Shadow
	b32 draw_shadow = sprite->draw_shadow;
	Color shadow_color = sprite->shadow_color;
	v2 shadow_scale = sprite->shadow_scale;
	v2 shadow_offset = sprite->shadow_offset;

	*current_idx = (struct SpriteToDraw){.sprite_frame = sprite_frame,
					     .layer = sprite->layer,
					     .order_in_layer = sprite->order_in_layer,
					     .position = position,
					     .height = -transform->virtual_height,
					     .rotation = rotation,
					     .scale = scale,
					     .color = sprite->color,
					     .flipX = sprite->flipX,
					     .flipY = sprite->flipY,
					     .pivot_override = sprite->pivot_override,
					     .texture = texture,
					     .draw_shadow = draw_shadow,
					     .shadow_color = shadow_color,
					     .shadow_offset = shadow_offset,
					     .shadow_scale = shadow_scale};
	current_idx++;
    }
    u32 total_sprites = current_idx - ordered_sprites_to_draw;

    qsort(ordered_sprites_to_draw, total_sprites, sizeof(struct SpriteToDraw), cmp_sprite_order);

    // Draw shadows
    for (usize i = 0; i < total_sprites; ++i)
    {
	struct SpriteToDraw *sprite_to_draw = &ordered_sprites_to_draw[i];
	if (sprite_to_draw->draw_shadow)
	{
	    v2 shadow_final_offset = Vector2Add(
		Vector2Multiply(sprite_to_draw->shadow_offset,
				sprite_to_draw->sprite_frame->source_size),
		(v2){.x = -sprite_to_draw->height * 0.1f, .y = -sprite_to_draw->height * 0.1f});
	    render_sprite_draw(sprite_to_draw->sprite_frame,
			       Vector2Add(sprite_to_draw->position, shadow_final_offset),
			       sprite_to_draw->rotation - 45,
			       Vector2Multiply(sprite_to_draw->shadow_scale, sprite_to_draw->scale),
			       sprite_to_draw->shadow_color, sprite_to_draw->flipX, true,
			       sprite_to_draw->pivot_override, sprite_to_draw->texture);
	}
    }

    // Draw sprites
    for (usize i = 0; i < total_sprites; ++i)
    {
	struct SpriteToDraw *sprite_to_draw = &ordered_sprites_to_draw[i];
	render_sprite_draw(sprite_to_draw->sprite_frame,
			   Vector2Add(sprite_to_draw->position, (v2){.y = sprite_to_draw->height}),
			   sprite_to_draw->rotation, sprite_to_draw->scale, sprite_to_draw->color,
			   sprite_to_draw->flipX, sprite_to_draw->flipY,
			   sprite_to_draw->pivot_override, sprite_to_draw->texture);
    }
}
