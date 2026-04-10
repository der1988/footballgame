#ifndef SPRITES_H
#define SPRITES_H

#include "as_sprite_frame.h"
#include "as_texture.h"

enum sprite_layer {
	SL_BACKGROUND = -100,
	SL_NORTH_GOAL = -10,
	SL_DEFAULT = 0,
	SL_PLAYERS_AND_BALL = 10,
	SL_SOUTH_GOAL = 20
};

enum sprite_orientation {
	SO_UP,
	SO_DOWN,
	SO_LEFT,
	SO_RIGHT,
	SO_UP_LEFT,
	SO_UP_RIGHT,
	SO_DOWN_LEFT,
	SO_DOWN_RIGHT,
	SO_COUNT
};

extern const char *sprite_orientation_to_string[SO_COUNT];
const char *sprite_layer_to_string(enum sprite_layer layer);

void render_sprite_draw(struct asset_sprite_frame *sprite_frame,
						v2 position,
						f32 rotation,
						v2 scale,
						Color color,
						b32 flipX,
						b32 flipY,
						v2 pivot_override,
						struct asset_texture *assets_texture);

void render_sprite_draw_debug_frame(struct asset_sprite_frame *sprite_frame,
									v2 position,
									f32 rotation,
									v2 scale,
									v2 pivot_override);

#endif // !SPRITES_H
