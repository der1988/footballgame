#ifndef GFX_H
#define GFX_H

#include "types.h"

struct game_state;
struct entity;

color_t gfx_color_lerp(color_t from, color_t to, f32 t);


void gfx_player_prepare(struct game_state *game_state);
void gfx_goalie_prepare(struct game_state *game_state);

#endif // !GFX_H
