#ifndef SPRITES_H
#define SPRITES_H

#include "assets.h"

enum sprite_layer {
    SL_BACKGROUND = -100,
    SL_NORTH_GOAL = -10,
    SL_DEFAULT = 0,
    SL_PLAYERS_AND_BALL = 10,
    SL_SOUTH_GOAL = 20
};

void render_sprite_draw(struct assets_sprite_frame *sprite_frame,
			v2 position,
			f32 rotation,
			v2 scale,
			Color color,
			b32 flipX,
			b32 flipY,
			v2 pivot_override,
			struct assets_texture *assets_texture);

void render_sprite_draw_debug_frame(struct assets_sprite_frame *sprite_frame,
				    v2 position,
				    f32 rotation,
				    v2 scale,
				    v2 pivot_override);

#endif // !SPRITES_H
