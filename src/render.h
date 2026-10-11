#ifndef RENDER_H
#define RENDER_H

#include "asset_sprite_frame.h"
#include "asset_texture.h"

enum sprite_layer {
	SL_BACKGROUND = -100,
	SL_NORTH_GOAL = -10,
	SL_DEFAULT = 0,
	SL_PLAYERS_AND_BALL = 10,
	SL_SOUTH_GOAL = 20,
	SL_FOREGROUND = 100
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

// What DrawTexturePro needs to put a sprite frame on the screen
struct sprite_placement {
	// The frame in the spritesheet. A negative width or height draws it flipped on that axis.
	rect_t src;
	// The frame goes in this rectangle, moved up and left by origin
	rect_t dst;
	// The point of the rectangle that lands on dst.x, dst.y. The sprite rotates around it.
	v2 origin;
};

struct sprite_placement render_sprite_placement(struct asset_sprite_frame *sprite_frame,
												v2 position,
												v2 scale,
												b32 flipX,
												b32 flipY,
												v2 pivot_override);

void render_sprite_draw(struct asset_sprite_frame *sprite_frame,
						v2 position,
						f32 rotation,
						v2 scale,
						Color color,
						b32 flipX,
						b32 flipY,
						v2 pivot_override,
						struct asset_texture *assets_texture);

void render_sprite_draw_placed(const struct sprite_placement *placement,
							   f32 rotation,
							   Color color,
							   struct asset_texture *assets_texture);

void render_sprite_draw_debug_frame(struct asset_sprite_frame *sprite_frame,
									v2 position,
									f32 rotation,
									v2 scale,
									v2 pivot_override);

#endif // !RENDER_H
