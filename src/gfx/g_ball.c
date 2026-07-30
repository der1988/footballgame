#include "game/game.h"

void gfx_ball_prepare(struct game_state *game_state)
{
	asset_animation_cache_t *animation_cache = game_state->assets.animation_cache;
	memory_arena_handle_t *memory_arena_handle = game_state->memory_by_lifetime[MAL_UPDATE];

	struct ball *ball = &game_state->gfx.ball;

	// Roll frames
	ball->roll_animations[SO_UP] =
		asset_animation_id_from_name(animation_cache, "ball", "up", memory_arena_handle);
	ball->roll_animations[SO_DOWN] =
		asset_animation_id_from_name(animation_cache, "ball", "down", memory_arena_handle);
	ball->roll_animations[SO_LEFT] =
		asset_animation_id_from_name(animation_cache, "ball", "left", memory_arena_handle);
	ball->roll_animations[SO_RIGHT] =
		asset_animation_id_from_name(animation_cache, "ball", "right", memory_arena_handle);
	ball->roll_animations[SO_UP_LEFT] =
		asset_animation_id_from_name(animation_cache, "ball", "up_left", memory_arena_handle);
	ball->roll_animations[SO_UP_RIGHT] =
		asset_animation_id_from_name(animation_cache, "ball", "up_right", memory_arena_handle);
	ball->roll_animations[SO_DOWN_LEFT] =
		asset_animation_id_from_name(animation_cache, "ball", "down_left", memory_arena_handle);
	ball->roll_animations[SO_DOWN_RIGHT] =
		asset_animation_id_from_name(animation_cache, "ball", "down_right", memory_arena_handle);
}
