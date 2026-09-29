#ifndef GAME_H
#define GAME_H

#include "asset.h"
#include "entity_ball.h"
#include "entity_player.h"
#include "events_router.h"
#include "football.h"
#include "gameplay.h"
#include "input.h"
#include "memory.h"
#include "types.h"
#include "layer.h"
#include "constants.h"

// Forward declarations
struct entity_repository;
struct input;
struct vcrui;
struct tweakui;
struct entityui;

enum memory_arena_lifetime {
	MAL_LEVEL,
	MAL_UPDATE,
	MAL_DRAW,
	MAL_TIME_MACHINE,
	MAL_COUNT
};

struct game_config {
	u32 virtual_screen_width;
	u32 virtual_screen_height;
	u32 screen_width;
	u32 screen_height;
	enum gameplay_mode gameplay_mode;
};

struct game_constants {
	f64 fixed_timestep;
	b32 ai_enabled;
	b32 show_radar;
	b32 shoot_aid_auto_aim;
	b32 shoot_aid_no_inertia;

	v3 gravity_force;
	f32 pixels_per_meter;
	f32 air_density;
	f32 crossbar_height;

	// Coefficient of restitution
	f32 ball_cr;
	f32 ball_cr_against_post;
	f32 ball_drag_coefficient;
	f32 ball_cross_sectional_area;
	u8 ball_rolling_resistance_mult;
	f32 ball_run_force;
	f32 ball_control_force;
	f32 ball_shot_force;
	f32 ball_shot_height;
	f32 ball_chip_shot_force;
	f32 ball_chip_shot_height;
	f32 ball_pass_force;
	f32 ball_pass_height;
	f32 ball_pass_auto_max_distance;
	f32 ball_pass_auto_lead_amount;
	f32 ball_long_pass_force;
	f32 ball_long_pass_height;
	f32 ball_long_driven_pass_force;
	f32 ball_long_driven_pass_height;
	f32 ball_tackle_force;
	f32 ball_sliding_tackle_force;
	f32 ball_header_fumble_force;
	f32 ball_header_min_force;
	f32 ball_max_velocity;
	f32 ball_max_velocity_to_gain_possession;
	u32 ball_lookaheads;
	u32 ball_lookahead_dts;
	f32 ball_after_touch_delay_time;
	f32 ball_after_touch_max_time;
	f32 ball_after_touch_force;

	// Following constants might become variables that depends on player characteristics at some
	// point
	f32 player_movement_force;
	f32 player_movement_drag;
	f32 player_control_max_velocity;
	f32 player_run_max_velocity;
	f32 player_control_radius;
	f32 player_control_height;
	f32 player_tackle_friction_force;
	f32 player_sliding_tackle_friction_force;
	// A turn is skidded rather than run round only above this speed and when the direction
	// asked for is turned this far from the run: -1 is a straight reversal, 0 a right angle,
	// so a value just above 0 takes in a square cut as well.
	f32 player_dribble_min_velocity;
	f32 player_dribble_max_alignment;
	// The skid holds for min_time whatever the steering does next, so the pose gets long enough
	// on screen to read, and is dropped at max_time however the turn is going.
	f32 player_dribble_min_time;
	f32 player_dribble_max_time;
	f32 player_goal_side_bonus;
	f32 player_switch_grace_time;
	// Tackled run: how long the player keeps running straight after a tackle, and the slowest
	// speed he runs at.
	f32 player_tackled_run_time;
	f32 player_tackled_run_min_velocity;
	// Steering for AI and assisted players: brakes onto a target at brake_ratio of the player's
	// full push, and only eases off the stick once his velocity is within band of the one it wants.
	f32 player_steering_brake_ratio;
	f32 player_steering_band;

	f32 goalie_dive_reaction_time;
	f32 goalie_dive_force;
	f32 goalie_dive_friction_force;
	f32 goalie_dive_max_velocity;
	f32 goalie_dive_min_ball_velocity;
	f32 goalie_dive_recover_time;
	f32 goalie_max_reachable_height;

	f32 kick_charge_reset_time;
};

struct human_player {
	b32 enabled;
	enum team_side team_side;
	game_controller_id_t game_controller_id;
	struct {
		entity_id_t entity_id;
		b32 is_manually_selected;
		fg_time_t last_switch_time;
	} controlled_player;
};

struct team_play_context {
	enum pitch_side pitch_side;
	enum team_formation formation;
	entity_id_t player_nearest_to_the_ball;
	entity_id_t player_in_charge_of_pressing;
};

enum game_mode {
	GM_TICK,
	GM_VCR,
};

struct game_vars {
	enum game_mode mode;
	struct pitch pitch;
	struct team {
		struct team_info info;
		struct team_play_context play_context;
	} teams[TS_COUNT];
	struct {
		b32 show_crosshair;
	} hud;
	struct entity *camera_entity;
	struct {
		struct entity *entity;
		v3 lookahead_positions[BALL_MAX_LOOKAHEADS];
		// Who had the ball before any system ran this tick
		struct ball_possession possession_at_tick_start;
	} ball;
	b32 is_debug_log_shown;
};

struct game_gfx {
	struct player {
		asset_sprite_frame_id_t stand_frames[SO_COUNT];
		asset_animation_id_t run_animations[SO_COUNT];
		asset_animation_id_t run_tackled_animations[SO_COUNT];
		asset_sprite_frame_id_t tackle_frames[SO_COUNT];
		asset_sprite_frame_id_t sliding_tackle_frames[SO_COUNT];
		asset_sprite_frame_id_t dribble_frames[PDT_COUNT];
		asset_animation_id_t fall_animations[SO_COUNT];
		asset_animation_id_t header_animations[SO_COUNT];
	} player;

	struct goalie {
		asset_sprite_frame_id_t stand_frames[SO_COUNT];
		asset_animation_id_t run_animations[SO_COUNT];
		asset_animation_id_t run_ball_animations[SO_COUNT];
		asset_animation_id_t hand_pass_animations[SO_COUNT];
		asset_sprite_frame_id_t save_frames[PSI_COUNT][ST_COUNT];
		asset_sprite_frame_id_t dash_frames[PSI_COUNT][SD_COUNT][ST_COUNT];
		asset_animation_id_t dive_animations_start[PSI_COUNT][SD_COUNT][SH_COUNT];
		asset_animation_id_t dive_animations_end[PSI_COUNT][SD_COUNT][SH_COUNT][ST_COUNT];
		asset_animation_id_t jump_animations_start[PSI_COUNT];
		asset_animation_id_t jump_animations_end[PSI_COUNT][ST_COUNT];
	} goalie;

	struct ball {
		asset_sprite_frame_id_t roll_animations[SO_COUNT];
	} ball;

	struct {
		asset_sprite_frame_id_t crosshair;
	} hud;
};

//	"player", "goalie", "dive", "tackle_1", "tackle_2", "header", "body"};
struct debug_flags {
	b32 debug_render;
	b32 show_bounding_boxes;

	b32 show_pitch_lines;

	b32 show_ball_future_bounce_position;
	b32 show_ball_lookahead_positions;
	b32 show_ball_collider;

	b32 show_goal_collider;

	b32 show_player_collider[PCT_COUNT];
	b32 show_player_steering;
};

struct game_state {
	b32 initialised;
	// Time we get from platform at the start of the frame.
	fg_time_t current_time;
	f64 accumulator;
	// Time that should be used for anything inside the game.
	fg_time_t time;
	delta_time_t dt;
	u64 physics_updates;
	u64 frame_updates;
	struct debug_flags debug_flags;
	struct game_config config;
	struct game_constants constants;
	struct game_vars vars;
	struct gameplay gameplay;
	struct game_gfx gfx;
	struct human_player human_players[MAX_HUMAN_PLAYERS];
	memory_arena_handle_t *memory_by_lifetime[MAL_COUNT];
	struct assets assets;
	struct entity_repository *entity_repository;
	events_router_t *events_router;
	struct debugui *debugui;
	layer_debug_t *layer_debug;
	layer_hud_t *layer_hud;
	struct asset_texture *pitch_texture;
	render_texture_t render_texture;
	camera2d_t camera;
	camera2d_t virtual_camera;
	struct time_machine *time_machine;
	struct input_manager *input_manager;
};

struct game_state *game_play_init(struct game_config gameConfig,
								  struct assets assets,
								  memory_arena_handle_t *memory_arena);
void game_play_destroy(struct game_state *game_state);
b32 game_play_update(struct game_state *game_state);
void game_play_draw_scene_to_texture(struct game_state *game_state,
									 render_texture_t target,
									 camera2d_t cam,
									 b32 draw_debug);

enum game_scene {
	GSC_NONE,
	GSC_TITLE,
	GSC_MAIN,
	GSC_SETTINGS,
	GSC_PLAY,
};

void game_loop(struct game_config config,
			   struct assets assets,
			   memory_arena_handle_t *memory_arena);

void game_set_next_scene(enum game_scene next_scene);

INLINE struct human_player *game_human_player_by_id(struct game_state *game_state,
													human_player_id_t id)
{
	if (game_state->human_players[id].enabled) {
		return &game_state->human_players[id];
	}
	return NULL;
}

INLINE usize game_human_players_count_by_team(struct game_state *game_state,
											  enum team_side team_side)
{
	usize count = 0;
	for (usize i = 0; i < MAX_HUMAN_PLAYERS; ++i) {
		if (game_state->human_players[i].enabled &&
			game_state->human_players[i].team_side == team_side) {
			count++;
		}
	}
	return count;
}

INLINE struct human_player *game_human_player_by_game_controller_id(
	struct game_state *game_state, game_controller_id_t game_controller_id)
{
	for (usize i = 0; i < MAX_HUMAN_PLAYERS; ++i) {
		if (game_state->human_players[i].enabled &&
			game_state->human_players[i].game_controller_id == game_controller_id) {
			return &game_state->human_players[i];
		}
	}
	return NULL;
}

INLINE struct human_player *game_input_human_player_by_player_entity_id(
	struct game_state *game_state, entity_id_t player_entity_id)
{
	for (usize i = 0; i < MAX_HUMAN_PLAYERS; ++i) {
		if (game_state->human_players[i].enabled &&
			game_state->human_players[i].controlled_player.entity_id == player_entity_id) {
			return &game_state->human_players[i];
		}
	}
	return NULL;
}

#endif // !GAME_H
