#include "../assets/assets.h"
#include "../utils/utils.h"
#include <raylib.h>
#include <raymath.h>

void sprites_draw(AssetsSpriteFrame *sprite_frame, Vector2 position, f32 rotation, Vector2 scale,
                  Color color, b32 flipX, b32 flipY, AssetsTexture *assets_texture) {
    Vector2 pivot = sprite_frame->pivot;

    // Draw texture
    Rectangle src = sprite_frame->frame;

    Vector2 origin = {sprite_frame->source_size.x * scale.x * pivot.x -
                          sprite_frame->sprite_source_size.x * scale.x,
                      sprite_frame->source_size.y * scale.y * pivot.y -
                          sprite_frame->sprite_source_size.y * scale.y};

    Rectangle dst = {position.x, position.y, sprite_frame->sprite_source_size.width * scale.x,
                     sprite_frame->sprite_source_size.height * scale.y};

    if (flipX) {
        src.width *= -1;
        origin.x = sprite_frame->source_size.x * scale.x * pivot.x -
                   (sprite_frame->source_size.x - sprite_frame->sprite_source_size.width -
                    sprite_frame->sprite_source_size.x) *
                       scale.x;
    }
    if (flipY) {
        src.height *= -1;
        origin.y = sprite_frame->source_size.y * scale.y * pivot.y -
                   (sprite_frame->source_size.y - sprite_frame->sprite_source_size.height -
                    sprite_frame->sprite_source_size.y) *
                       scale.y;
    }

    DrawTexturePro(assets_texture->texture, src, dst, origin, rotation, color);
}

static Vector2 rotate_vector_around_point(Vector2 vector, Vector2 pivot, f32 angle) {
    Vector2 new_vector = Vector2Subtract(pivot, vector);
    new_vector = Vector2Rotate(new_vector, DEG2RAD * (angle + 180));
    return Vector2Add(new_vector, pivot);
}

void sprites_draw_debug_frame(AssetsSpriteFrame *sprite_frame, Vector2 position, f32 rotation,
                              Vector2 scale) {
    Vector2 pivot = sprite_frame->pivot;

    // Draw pivot
    DrawCircleV(position, 2, RED);

    Vector2 top_left = {.x = position.x - sprite_frame->source_size.x * scale.x * pivot.x,
                        .y = position.y - sprite_frame->source_size.y * scale.y * pivot.y};
    Vector2 top_right =
        Vector2Add(top_left, Vector2Scale(Vector2Right(), sprite_frame->source_size.x * scale.x));
    Vector2 bottom_left =
        Vector2Add(top_left, Vector2Scale(Vector2Down(), sprite_frame->source_size.y * scale.y));
    Vector2 bottom_right = Vector2Add(
        bottom_left, Vector2Scale(Vector2Right(), sprite_frame->source_size.x * scale.x));

    // Draw debug frame
    DrawLineV(rotate_vector_around_point(top_left, position, rotation),
              rotate_vector_around_point(top_right, position, rotation), GREEN);
    DrawLineV(rotate_vector_around_point(top_left, position, rotation),
              rotate_vector_around_point(bottom_left, position, rotation), GREEN);
    DrawLineV(rotate_vector_around_point(bottom_left, position, rotation),
              rotate_vector_around_point(bottom_right, position, rotation), GREEN);
    DrawLineV(rotate_vector_around_point(top_right, position, rotation),
              rotate_vector_around_point(bottom_right, position, rotation), GREEN);
}
