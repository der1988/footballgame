#include "types.h"

struct game_state;

color_t gfx_color_lerp(color_t from, color_t to, f32 t);

void prepare_player_gfx(struct game_state *game_state);
void prepare_goalie_gfx(struct game_state *game_state);
