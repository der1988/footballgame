#include "game.h"

void prepare_goalie_gfx(struct game_state *game_state)
{
	struct goalie *goalie = &game_state->gfx.goalie;

	// Stand frames
	goalie->stand_frames[SO_DOWN] =
		assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "goalkeeper_front_0")
			->id;
	goalie->stand_frames[SO_UP] =
		assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "goalkeeper_back_0")
			->id;
	goalie->stand_frames[SO_LEFT] =
		assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "goalkeeper_left_0")
			->id;
	goalie->stand_frames[SO_RIGHT] =
		assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "goalkeeper_right_0")
			->id;
	goalie->stand_frames[SO_DOWN_LEFT] =
		assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
										"goalkeeper_front_left_0")
			->id;
	goalie->stand_frames[SO_DOWN_RIGHT] =
		assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
										"goalkeeper_front_right_0")
			->id;
	goalie->stand_frames[SO_UP_LEFT] =
		assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
										"goalkeeper_back_left_0")
			->id;
	goalie->stand_frames[SO_UP_RIGHT] =
		assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
										"goalkeeper_back_right_0")
			->id;

	assets_animation_cache_t *animation_cache = game_state->assets.animation_cache;
	memory_arena_handle_t *memory_arena_handle = game_state->memory_by_lifetime[MAL_UPDATE];

	// Running animations
	goalie->run_animations[SO_DOWN] = assets_animation_id_from_name(
		animation_cache, "goalkeeper", "run_front", memory_arena_handle);
	goalie->run_animations[SO_UP] = assets_animation_id_from_name(
		animation_cache, "goalkeeper", "run_back", memory_arena_handle);
	goalie->run_animations[SO_LEFT] = assets_animation_id_from_name(
		animation_cache, "goalkeeper", "run_left", memory_arena_handle);
	goalie->run_animations[SO_RIGHT] = assets_animation_id_from_name(
		animation_cache, "goalkeeper", "run_right", memory_arena_handle);
	goalie->run_animations[SO_DOWN_LEFT] = assets_animation_id_from_name(
		animation_cache, "goalkeeper", "run_front_left", memory_arena_handle);
	goalie->run_animations[SO_DOWN_RIGHT] = assets_animation_id_from_name(
		animation_cache, "goalkeeper", "run_front_right", memory_arena_handle);
	goalie->run_animations[SO_UP_LEFT] = assets_animation_id_from_name(
		animation_cache, "goalkeeper", "run_back_left", memory_arena_handle);
	goalie->run_animations[SO_UP_RIGHT] = assets_animation_id_from_name(
		animation_cache, "goalkeeper", "run_back_right", memory_arena_handle);

	// Save frames
	goalie->save_frames[PSI_NORTH][ST_WITHOUT_BALL] =
		assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "goalkeeper_front_0")
			->id;
	goalie->save_frames[PSI_NORTH][ST_WITH_BALL] =
		assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
										"goalkeeper_front_ball_0")
			->id;
	goalie->save_frames[PSI_SOUTH][ST_WITHOUT_BALL] =
		assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, "goalkeeper_back_0")
			->id;
	goalie->save_frames[PSI_SOUTH][ST_WITH_BALL] =
		assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
										"goalkeeper_back_ball_0")
			->id;

	// Dash frames
	goalie->dash_frames[PSI_NORTH][SD_LEFT][ST_WITHOUT_BALL] =
		assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
										"goalkeeper_dash_front_left_0")
			->id;
	goalie->dash_frames[PSI_NORTH][SD_LEFT][ST_WITH_BALL] =
		assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
										"goalkeeper_dash_front_left_1")
			->id;
	goalie->dash_frames[PSI_NORTH][SD_RIGHT][ST_WITHOUT_BALL] =
		assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
										"goalkeeper_dash_front_right_0")
			->id;
	goalie->dash_frames[PSI_NORTH][SD_RIGHT][ST_WITH_BALL] =
		assets_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache,
										"goalkeeper_dash_front_right_1")
			->id;

	// Diving animations
	goalie->dive_animations_start[PSI_NORTH][SD_LEFT][SH_HIGH] = assets_animation_id_from_name(
		animation_cache, "goalkeeper", "dive_front_high_left_start", memory_arena_handle);
	goalie->dive_animations_start[PSI_NORTH][SD_RIGHT][SH_HIGH] = assets_animation_id_from_name(
		animation_cache, "goalkeeper", "dive_front_high_right_start", memory_arena_handle);
	goalie->dive_animations_start[PSI_NORTH][SD_LEFT][SH_LOW] = assets_animation_id_from_name(
		animation_cache, "goalkeeper", "dive_front_low_left_start", memory_arena_handle);
	goalie->dive_animations_start[PSI_NORTH][SD_RIGHT][SH_LOW] = assets_animation_id_from_name(
		animation_cache, "goalkeeper", "dive_front_low_right_start", memory_arena_handle);

	goalie->dive_animations_end[PSI_NORTH][SD_LEFT][SH_HIGH][ST_WITHOUT_BALL] =
		assets_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_front_high_left_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_NORTH][SD_RIGHT][SH_HIGH][ST_WITHOUT_BALL] =
		assets_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_front_high_right_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_NORTH][SD_LEFT][SH_LOW][ST_WITHOUT_BALL] =
		assets_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_front_low_left_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_NORTH][SD_RIGHT][SH_LOW][ST_WITHOUT_BALL] =
		assets_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_front_low_right_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_NORTH][SD_LEFT][SH_HIGH][ST_WITH_BALL] =
		assets_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_front_high_left_ball_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_NORTH][SD_RIGHT][SH_HIGH][ST_WITH_BALL] =
		assets_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_front_high_right_ball_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_NORTH][SD_LEFT][SH_LOW][ST_WITH_BALL] =
		assets_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_front_low_left_ball_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_NORTH][SD_RIGHT][SH_LOW][ST_WITH_BALL] =
		assets_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_front_low_right_ball_end", memory_arena_handle);

	goalie->dive_animations_start[PSI_SOUTH][SD_LEFT][SH_HIGH] = assets_animation_id_from_name(
		animation_cache, "goalkeeper", "dive_back_high_left_start", memory_arena_handle);
	goalie->dive_animations_start[PSI_SOUTH][SD_RIGHT][SH_HIGH] = assets_animation_id_from_name(
		animation_cache, "goalkeeper", "dive_back_high_right_start", memory_arena_handle);
	goalie->dive_animations_start[PSI_SOUTH][SD_LEFT][SH_LOW] = assets_animation_id_from_name(
		animation_cache, "goalkeeper", "dive_back_low_left_start", memory_arena_handle);
	goalie->dive_animations_start[PSI_SOUTH][SD_RIGHT][SH_LOW] = assets_animation_id_from_name(
		animation_cache, "goalkeeper", "dive_back_low_right_start", memory_arena_handle);

	goalie->dive_animations_end[PSI_SOUTH][SD_LEFT][SH_HIGH][ST_WITHOUT_BALL] =
		assets_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_back_high_left_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_SOUTH][SD_RIGHT][SH_HIGH][ST_WITHOUT_BALL] =
		assets_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_back_high_right_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_SOUTH][SD_LEFT][SH_LOW][ST_WITHOUT_BALL] =
		assets_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_back_low_left_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_SOUTH][SD_RIGHT][SH_LOW][ST_WITHOUT_BALL] =
		assets_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_back_low_right_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_SOUTH][SD_LEFT][SH_HIGH][ST_WITH_BALL] =
		assets_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_back_high_left_ball_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_SOUTH][SD_RIGHT][SH_HIGH][ST_WITH_BALL] =
		assets_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_back_high_right_ball_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_SOUTH][SD_LEFT][SH_LOW][ST_WITH_BALL] =
		assets_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_back_low_left_ball_end", memory_arena_handle);
	goalie->dive_animations_end[PSI_SOUTH][SD_RIGHT][SH_LOW][ST_WITH_BALL] =
		assets_animation_id_from_name(
			animation_cache, "goalkeeper", "dive_back_low_right_ball_end", memory_arena_handle);

	// Jump animations
	goalie->jump_animations_start[PSI_NORTH] = assets_animation_id_from_name(
		animation_cache, "goalkeeper", "front_jump_parry_start", memory_arena_handle);
	goalie->jump_animations_start[PSI_SOUTH] = assets_animation_id_from_name(
		animation_cache, "goalkeeper", "back_jump_parry_start", memory_arena_handle);
	goalie->jump_animations_end[PSI_NORTH][ST_WITHOUT_BALL] = assets_animation_id_from_name(
		animation_cache, "goalkeeper", "front_jump_parry_end", memory_arena_handle);
	goalie->jump_animations_end[PSI_NORTH][ST_WITH_BALL] = assets_animation_id_from_name(
		animation_cache, "goalkeeper", "front_jump_catch_end", memory_arena_handle);
	goalie->jump_animations_end[PSI_SOUTH][ST_WITHOUT_BALL] = assets_animation_id_from_name(
		animation_cache, "goalkeeper", "back_jump_parry_end", memory_arena_handle);
	goalie->jump_animations_end[PSI_SOUTH][ST_WITH_BALL] = assets_animation_id_from_name(
		animation_cache, "goalkeeper", "back_jump_catch_end", memory_arena_handle);
}
