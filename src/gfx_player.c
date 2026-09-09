#include "gfx.h"
#include "game.h"

void gfx_player_prepare(struct game_state *game_state)
{
	asset_animation_cache_t *animation_cache = game_state->assets.animation_cache;
	memory_arena_handle_t *memory_arena_handle = game_state->memory_by_lifetime[MAL_UPDATE];

	struct player *player = &game_state->gfx.player;

	// Stand frames
	player->stand_frames[SO_DOWN] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "player_front_0")->id;
	player->stand_frames[SO_UP] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "player_back_0")->id;
	player->stand_frames[SO_LEFT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "player_left_0")->id;
	player->stand_frames[SO_RIGHT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "player_right_0")->id;
	player->stand_frames[SO_DOWN_LEFT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "player_front_left_0")
			->id;
	player->stand_frames[SO_DOWN_RIGHT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "player_front_right_0")
			->id;
	player->stand_frames[SO_UP_LEFT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "player_back_left_0")
			->id;
	player->stand_frames[SO_UP_RIGHT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "player_back_right_0")
			->id;

	// Sliding tackle frames
	player->sliding_tackle_frames[SO_DOWN] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "player_tackle_slide_front_0")
			->id;
	player->sliding_tackle_frames[SO_UP] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "player_tackle_slide_back_0")
			->id;
	player->sliding_tackle_frames[SO_LEFT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "player_tackle_slide_left_0")
			->id;
	player->sliding_tackle_frames[SO_RIGHT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "player_tackle_slide_right_0")
			->id;
	player->sliding_tackle_frames[SO_DOWN_LEFT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "player_tackle_slide_front_left_0")
			->id;
	player->sliding_tackle_frames[SO_DOWN_RIGHT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "player_tackle_slide_front_right_0")
			->id;
	player->sliding_tackle_frames[SO_UP_LEFT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "player_tackle_slide_back_left_0")
			->id;
	player->sliding_tackle_frames[SO_UP_RIGHT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "player_tackle_slide_back_right_0")
			->id;

	// Dribble frames
	player->dribble_frames[PDT_FRONT_TO_BACK_RIGHT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "player_dribble_FtoB_right_0")
			->id;
	player->dribble_frames[PDT_FRONT_TO_BACK_LEFT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "player_dribble_FtoB_left_0")
			->id;
	player->dribble_frames[PDT_BACK_TO_FRONT_RIGHT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "player_dribble_BtoF_right_0")
			->id;
	player->dribble_frames[PDT_BACK_TO_FRONT_LEFT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "player_dribble_BtoF_left_0")
			->id;
	player->dribble_frames[PDT_RIGHT_TO_LEFT_FRONT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "player_dribble_RtoL_front_0")
			->id;
	player->dribble_frames[PDT_RIGHT_TO_LEFT_BACK] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "player_dribble_RtoL_back_0")
			->id;
	player->dribble_frames[PDT_LEFT_TO_RIGHT_FRONT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "player_dribble_LtoR_front_0")
			->id;
	player->dribble_frames[PDT_LEFT_TO_RIGHT_BACK] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "player_dribble_LtoR_back_0")
			->id;

	// Running animations
	player->run_animations[SO_DOWN] =
		asset_animation_id_from_name(animation_cache, "player", "run_front", memory_arena_handle);
	player->run_animations[SO_UP] =
		asset_animation_id_from_name(animation_cache, "player", "run_back", memory_arena_handle);
	player->run_animations[SO_LEFT] =
		asset_animation_id_from_name(animation_cache, "player", "run_left", memory_arena_handle);
	player->run_animations[SO_RIGHT] =
		asset_animation_id_from_name(animation_cache, "player", "run_right", memory_arena_handle);
	player->run_animations[SO_DOWN_LEFT] = asset_animation_id_from_name(
		animation_cache, "player", "run_front_left", memory_arena_handle);
	player->run_animations[SO_DOWN_RIGHT] = asset_animation_id_from_name(
		animation_cache, "player", "run_front_right", memory_arena_handle);
	player->run_animations[SO_UP_LEFT] = asset_animation_id_from_name(
		animation_cache, "player", "run_back_left", memory_arena_handle);
	player->run_animations[SO_UP_RIGHT] = asset_animation_id_from_name(
		animation_cache, "player", "run_back_right", memory_arena_handle);

	// Fall animations
	player->fall_animations[SO_DOWN] =
		asset_animation_id_from_name(animation_cache, "player", "fall_front", memory_arena_handle);
	player->fall_animations[SO_UP] =
		asset_animation_id_from_name(animation_cache, "player", "fall_back", memory_arena_handle);
	player->fall_animations[SO_LEFT] =
		asset_animation_id_from_name(animation_cache, "player", "fall_left", memory_arena_handle);
	player->fall_animations[SO_RIGHT] =
		asset_animation_id_from_name(animation_cache, "player", "fall_right", memory_arena_handle);
	player->fall_animations[SO_DOWN_LEFT] = asset_animation_id_from_name(
		animation_cache, "player", "fall_front_left", memory_arena_handle);
	player->fall_animations[SO_DOWN_RIGHT] = asset_animation_id_from_name(
		animation_cache, "player", "fall_front_right", memory_arena_handle);
	player->fall_animations[SO_UP_LEFT] = asset_animation_id_from_name(
		animation_cache, "player", "fall_back_left", memory_arena_handle);
	player->fall_animations[SO_UP_RIGHT] = asset_animation_id_from_name(
		animation_cache, "player", "fall_back_right", memory_arena_handle);

	// Header animations
	player->header_animations[SO_DOWN] = asset_animation_id_from_name(
		animation_cache, "player", "header_front", memory_arena_handle);
	player->header_animations[SO_UP] =
		asset_animation_id_from_name(animation_cache, "player", "header_back", memory_arena_handle);
	player->header_animations[SO_LEFT] =
		asset_animation_id_from_name(animation_cache, "player", "header_left", memory_arena_handle);
	player->header_animations[SO_RIGHT] = asset_animation_id_from_name(
		animation_cache, "player", "header_right", memory_arena_handle);
	player->header_animations[SO_DOWN_LEFT] = asset_animation_id_from_name(
		animation_cache, "player", "header_front_left", memory_arena_handle);
	player->header_animations[SO_DOWN_RIGHT] = asset_animation_id_from_name(
		animation_cache, "player", "header_front_right", memory_arena_handle);
	player->header_animations[SO_UP_LEFT] = asset_animation_id_from_name(
		animation_cache, "player", "header_back_left", memory_arena_handle);
	player->header_animations[SO_UP_RIGHT] = asset_animation_id_from_name(
		animation_cache, "player", "header_back_right", memory_arena_handle);
}
