#include "gfx.h"
#include "game.h"

void gfx_goalie_prepare(struct game_state *game_state)
{
	struct goalie *goalie = &game_state->gfx.goalie;

	// Stand frames
	goalie->stand_frames[SO_DOWN] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "goalkeeper_front_0")
			->id;
	goalie->stand_frames[SO_UP] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "goalkeeper_back_0")
			->id;
	goalie->stand_frames[SO_LEFT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "goalkeeper_left_0")
			->id;
	goalie->stand_frames[SO_RIGHT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "goalkeeper_right_0")
			->id;
	goalie->stand_frames[SO_DOWN_LEFT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "goalkeeper_front_left_0")
			->id;
	goalie->stand_frames[SO_DOWN_RIGHT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "goalkeeper_front_right_0")
			->id;
	goalie->stand_frames[SO_UP_LEFT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "goalkeeper_back_left_0")
			->id;
	goalie->stand_frames[SO_UP_RIGHT] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "goalkeeper_back_right_0")
			->id;

	asset_animation_cache_t *animation_cache = game_state->assets.animation_cache;
	memory_arena_handle_t *memory_arena_handle = game_state->memory_by_lifetime[MAL_UPDATE];

	// Running animations
	goalie->run_animations[SO_DOWN] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "run_front", memory_arena_handle);
	goalie->run_animations[SO_UP] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "run_back", memory_arena_handle);
	goalie->run_animations[SO_LEFT] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "run_left", memory_arena_handle);
	goalie->run_animations[SO_RIGHT] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "run_right", memory_arena_handle);
	goalie->run_animations[SO_DOWN_LEFT] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "run_front_left", memory_arena_handle);
	goalie->run_animations[SO_DOWN_RIGHT] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "run_front_right", memory_arena_handle);
	goalie->run_animations[SO_UP_LEFT] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "run_back_left", memory_arena_handle);
	goalie->run_animations[SO_UP_RIGHT] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "run_back_right", memory_arena_handle);

	// Throwing the ball out by hand
	goalie->hand_pass_animations[SO_DOWN] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "hand_pass_front", memory_arena_handle);
	goalie->hand_pass_animations[SO_UP] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "hand_pass_back", memory_arena_handle);
	goalie->hand_pass_animations[SO_LEFT] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "hand_pass_left", memory_arena_handle);
	goalie->hand_pass_animations[SO_RIGHT] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "hand_pass_right", memory_arena_handle);
	goalie->hand_pass_animations[SO_DOWN_LEFT] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "hand_pass_front_left", memory_arena_handle);
	goalie->hand_pass_animations[SO_DOWN_RIGHT] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "hand_pass_front_right", memory_arena_handle);
	goalie->hand_pass_animations[SO_UP_LEFT] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "hand_pass_back_left", memory_arena_handle);
	goalie->hand_pass_animations[SO_UP_RIGHT] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "hand_pass_back_right", memory_arena_handle);

	// Running with the ball in his hands
	goalie->run_ball_animations[SO_DOWN] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "run_ball_front", memory_arena_handle);
	goalie->run_ball_animations[SO_UP] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "run_ball_back", memory_arena_handle);
	goalie->run_ball_animations[SO_LEFT] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "run_ball_left", memory_arena_handle);
	goalie->run_ball_animations[SO_RIGHT] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "run_ball_right", memory_arena_handle);
	goalie->run_ball_animations[SO_DOWN_LEFT] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "run_ball_front_left", memory_arena_handle);
	goalie->run_ball_animations[SO_DOWN_RIGHT] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "run_ball_front_right", memory_arena_handle);
	goalie->run_ball_animations[SO_UP_LEFT] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "run_ball_back_left", memory_arena_handle);
	goalie->run_ball_animations[SO_UP_RIGHT] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "run_ball_back_right", memory_arena_handle);

	// Save frames
	goalie->save_frames[PSI_NORTH][ST_WITHOUT_BALL] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "goalkeeper_front_0")
			->id;
	goalie->save_frames[PSI_NORTH][ST_WITH_BALL] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "goalkeeper_front_ball_0")
			->id;
	goalie->save_frames[PSI_SOUTH][ST_WITHOUT_BALL] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "goalkeeper_back_0")
			->id;
	goalie->save_frames[PSI_SOUTH][ST_WITH_BALL] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "goalkeeper_back_ball_0")
			->id;

	// Dash frames
	goalie->dash_frames[PSI_NORTH][SD_LEFT][ST_WITHOUT_BALL] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "goalkeeper_save_dash_front_left_0")
			->id;
	goalie->dash_frames[PSI_NORTH][SD_LEFT][ST_WITH_BALL] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "goalkeeper_save_dash_front_left_ball_0")
			->id;
	goalie->dash_frames[PSI_NORTH][SD_RIGHT][ST_WITHOUT_BALL] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "goalkeeper_save_dash_front_right_0")
			->id;
	goalie->dash_frames[PSI_NORTH][SD_RIGHT][ST_WITH_BALL] =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
									   "goalkeeper_save_dash_front_right_ball_0")
			->id;

	// Diving animations
	goalie->dive_animations_start[PSI_NORTH][SD_LEFT][SH_HIGH] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "dive_front_high_left_start", memory_arena_handle);
	goalie->dive_animations_start[PSI_NORTH][SD_RIGHT][SH_HIGH] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "dive_front_high_right_start", memory_arena_handle);
	goalie->dive_animations_start[PSI_NORTH][SD_LEFT][SH_LOW] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "dive_front_low_left_start", memory_arena_handle);
	goalie->dive_animations_start[PSI_NORTH][SD_RIGHT][SH_LOW] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "dive_front_low_right_start", memory_arena_handle);

	goalie->dive_animations_end[PSI_NORTH][SD_LEFT][SH_HIGH][ST_WITHOUT_BALL] =
		asset_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_front_high_left_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_NORTH][SD_RIGHT][SH_HIGH][ST_WITHOUT_BALL] =
		asset_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_front_high_right_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_NORTH][SD_LEFT][SH_LOW][ST_WITHOUT_BALL] =
		asset_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_front_low_left_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_NORTH][SD_RIGHT][SH_LOW][ST_WITHOUT_BALL] =
		asset_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_front_low_right_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_NORTH][SD_LEFT][SH_HIGH][ST_WITH_BALL] =
		asset_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_front_high_left_ball_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_NORTH][SD_RIGHT][SH_HIGH][ST_WITH_BALL] =
		asset_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_front_high_right_ball_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_NORTH][SD_LEFT][SH_LOW][ST_WITH_BALL] =
		asset_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_front_low_left_ball_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_NORTH][SD_RIGHT][SH_LOW][ST_WITH_BALL] =
		asset_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_front_low_right_ball_end", memory_arena_handle);

	goalie->dive_animations_start[PSI_SOUTH][SD_LEFT][SH_HIGH] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "dive_back_high_left_start", memory_arena_handle);
	goalie->dive_animations_start[PSI_SOUTH][SD_RIGHT][SH_HIGH] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "dive_back_high_right_start", memory_arena_handle);
	goalie->dive_animations_start[PSI_SOUTH][SD_LEFT][SH_LOW] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "dive_back_low_left_start", memory_arena_handle);
	goalie->dive_animations_start[PSI_SOUTH][SD_RIGHT][SH_LOW] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "dive_back_low_right_start", memory_arena_handle);

	goalie->dive_animations_end[PSI_SOUTH][SD_LEFT][SH_HIGH][ST_WITHOUT_BALL] =
		asset_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_back_high_left_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_SOUTH][SD_RIGHT][SH_HIGH][ST_WITHOUT_BALL] =
		asset_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_back_high_right_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_SOUTH][SD_LEFT][SH_LOW][ST_WITHOUT_BALL] =
		asset_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_back_low_left_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_SOUTH][SD_RIGHT][SH_LOW][ST_WITHOUT_BALL] =
		asset_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_back_low_right_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_SOUTH][SD_LEFT][SH_HIGH][ST_WITH_BALL] =
		asset_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_back_high_left_ball_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_SOUTH][SD_RIGHT][SH_HIGH][ST_WITH_BALL] =
		asset_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_back_high_right_ball_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_SOUTH][SD_LEFT][SH_LOW][ST_WITH_BALL] =
		asset_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_back_low_left_ball_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_SOUTH][SD_RIGHT][SH_LOW][ST_WITH_BALL] =
		asset_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_back_low_right_ball_end", memory_arena_handle);

	// Jump animations
	goalie->jump_animations_start[PSI_NORTH] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "front_jump_parry_start", memory_arena_handle);
	goalie->jump_animations_start[PSI_SOUTH] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "back_jump_parry_start", memory_arena_handle);
	goalie->jump_animations_end[PSI_NORTH][ST_WITHOUT_BALL] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "front_jump_parry_end", memory_arena_handle);
	goalie->jump_animations_end[PSI_NORTH][ST_WITH_BALL] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "front_jump_catch_end", memory_arena_handle);
	goalie->jump_animations_end[PSI_SOUTH][ST_WITHOUT_BALL] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "back_jump_parry_end", memory_arena_handle);
	goalie->jump_animations_end[PSI_SOUTH][ST_WITH_BALL] = asset_animation_id_from_name(
		animation_cache, "goalkeeper", "back_jump_catch_end", memory_arena_handle);
}
