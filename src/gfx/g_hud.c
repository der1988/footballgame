#include "as_sprite_frame.h"
#include "game.h"

void gfx_hud_prepare(struct game_state *game_state)
{
	game_state->gfx.hud.crosshair =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "crosshair_1_0")->id;
}
