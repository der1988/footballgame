#include "assets.h"
#include "utils.h"
#include <raylib.h>
#include <raymath.h>
#include "math_ext.h"

void render_sprite_draw(struct AssetsSpriteFrame *sprite_frame, v2 position, f32 rotation, v2 scale,
			Color color, b32 flipX, b32 flipY, v2 pivot_override,
			struct AssetsTexture *assets_texture)
{
    Vector2 pivot = Vector2Add(sprite_frame->pivot, pivot_override);

    // Draw texture
    Rectangle src = sprite_frame->frame;

    Vector2 origin = {sprite_frame->source_size.x * scale.x * pivot.x -
			  sprite_frame->sprite_source_size.x * scale.x,
		      sprite_frame->source_size.y * scale.y * pivot.y -
			  sprite_frame->sprite_source_size.y * scale.y};

    Rectangle dst = {position.x, position.y, sprite_frame->sprite_source_size.width * scale.x,
		     sprite_frame->sprite_source_size.height * scale.y};

    if (flipX)
    {
	src.width *= -1;
	origin.x = sprite_frame->source_size.x * scale.x * pivot.x -
		   (sprite_frame->source_size.x - sprite_frame->sprite_source_size.width -
		    sprite_frame->sprite_source_size.x) *
		       scale.x;
    }
    if (flipY)
    {
	src.height *= -1;
	origin.y = sprite_frame->source_size.y * scale.y * pivot.y -
		   (sprite_frame->source_size.y - sprite_frame->sprite_source_size.height -
		    sprite_frame->sprite_source_size.y) *
		       scale.y;
    }

    DrawTexturePro(assets_texture->texture, src, dst, origin, rotation, color);
}

void render_sprite_draw_debug_frame(struct AssetsSpriteFrame *sprite_frame, v2 position,
				    f32 rotation, v2 scale, v2 pivot_override)
{
    Vector2 pivot = Vector2Add(sprite_frame->pivot, pivot_override);

    // Draw pivot
    DrawCircleV(position, 1, RED);

    Vector2 top_left = {.x = position.x - sprite_frame->source_size.x * scale.x * pivot.x,
			.y = position.y - sprite_frame->source_size.y * scale.y * pivot.y};
    Vector2 top_right =
	Vector2Add(top_left, Vector2Scale(Vector2Right(), sprite_frame->source_size.x * scale.x));
    Vector2 bottom_left =
	Vector2Add(top_left, Vector2Scale(Vector2Down(), sprite_frame->source_size.y * scale.y));
    Vector2 bottom_right = Vector2Add(
	bottom_left, Vector2Scale(Vector2Right(), sprite_frame->source_size.x * scale.x));

    // Draw debug frame
    DrawLineV(Vector2RotateAroundPivot(top_left, position, rotation),
	      Vector2RotateAroundPivot(top_right, position, rotation), GREEN);
    DrawLineV(Vector2RotateAroundPivot(top_left, position, rotation),
	      Vector2RotateAroundPivot(bottom_left, position, rotation), GREEN);
    DrawLineV(Vector2RotateAroundPivot(bottom_left, position, rotation),
	      Vector2RotateAroundPivot(bottom_right, position, rotation), GREEN);
    DrawLineV(Vector2RotateAroundPivot(top_right, position, rotation),
	      Vector2RotateAroundPivot(bottom_right, position, rotation), GREEN);
}
