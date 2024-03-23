#ifndef SPRITES_H
#define SPRITES_H

#include "../assets/assets.h"

typedef enum SpriteLayer { SL_BACKGROUND = -100, SL_DEFAULT = 0, SL_PLAYERS = 10 } SpriteLayer;

void sprites_draw(AssetsSpriteFrame *sprite_frame, Vector2 position, f32 rotation, Vector2 scale,
                  Color color, b32 flipX, b32 flipY, AssetsTexture *texture);

void sprites_draw_debug_frame(AssetsSpriteFrame *sprite_frame, Vector2 position, f32 rotation,
                              Vector2 scale);

#endif // !SPRITES_H
