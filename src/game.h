#ifndef GAME_H
#define GAME_H

#include "assets.h"
#include "football.h"
#include "input.h"
#include "memory.h"
#include "types.h"
#include "layers.h"
#include "tweakui.h"
#include "constants.h"

// Forward declarations
struct entity_repository;
struct input;

enum memory_arena_lifetime {
	MAL_LEVEL,
	MAL_UPDATE,
	MAL_DRAW,
	MAL_COUNT
};

struct game_config {
	u32 virtual_screen_width;
	u32 virtual_screen_height;
	u32 screen_width;
	u32 screen_height;
};

struct game_constants {
	f64 fixed_timestep;
	b32 render_debug;
	b32 ai_enabled;
	b32 shoot_aid_no_inertia;
	f32 input_stick_dead_zone;
	v3 gravity_force;
	f32 pixels_per_meter;
	f32 air_density;
	u32 crossbar_height;

	// Coefficient of restitution
	f32 ball_cr;
	f32 ball_cr_against_post;
	f32 ball_drag_coefficient;
	f32 ball_cross_sectional_area;
	f32 ball_friction_on_pitch;
	f32 ball_run_force;
	f32 ball_control_force;
	f32 ball_shot_force;
	f32 ball_shot_height;
	f32 ball_pass_force;
	f32 ball_pass_height;
	f32 ball_long_pass_force;
	f32 ball_long_pass_height;
	f32 ball_tackle_force;
	f32 ball_after_touch_delay_time;
	f32 ball_after_touch_max_time;
	f32 ball_after_touch_force;
	f32 ball_max_velocity;
	f32 ball_max_velocity_to_gain_possession;
	u32 ball_lookaheads;
	u32 ball_lookahead_dts;

	// Following constants might become variables that depends on player characteristics at some
	// point
	f32 player_movement_force;
	f32 player_movement_drag;
	f32 player_control_max_velocity;
	f32 player_run_max_velocity;
	f32 player_control_radius;
	f32 player_control_height;
	f32 player_run_control_max_angle;

	f32 goalie_dive_reaction_time;
	f32 goalie_dive_force;
	f32 goalie_dive_max_velocity;
	f32 goalie_dive_min_ball_velocity;

	f32 kick_charge_reset_time;
};

struct device_controls {
	v2 steering;
	struct action_charge {
		enum action_charge_type {
			ACT_SHOOT,
			ACT_PASS,
			ACT_LONG_PASS
		} type;
		v2 direction;
		time_t started;
		time_t ended;
		f32 power;
		b32 charging;
		b32 ready;
	} charge;
};

struct human_player {
	enum team_side team_side;
	struct device_controls controls;
	entity_id_t controlled_player_entity_id;
};

struct game_vars {
	struct match match;
	struct pitch pitch;
	struct team teams[TS_COUNT];
	entity_id_t player_on_the_ball_entity_id;
	entity_id_t player_nearest_to_the_ball[TS_COUNT];
	entity_id_t camera_entity_id;
	entity_id_t ball_entity_id;
	b32 slow_motion;
	b32 is_debug_log_shown;
	b32 is_tweakui_shown;
};

struct game_state {
	b32 initialised;
	time_t current_time;
	f64 accumulator;
	time_t time;
	delta_time_t dt;
	u64 physics_updates;
	u64 frame_updates;
	struct game_config config;
	struct game_constants constants;
	struct game_vars vars;
	struct human_player human_players[MAX_HUMAN_PLAYERS];
	memory_arena_handle_t *memory_by_lifetime[MAL_COUNT];
	struct assets assets;
	struct entity_repository *entity_repository;
	tweakui_t *tweakui;
	layers_debug_t *layer_debug;
	assets_texture_id_t pitch_texture_id;
	render_texture_t render_texture;
	camera2d_t camera;
	camera2d_t virtual_camera;
};

struct game_state *game_play_init(struct game_config gameConfig,
								  struct assets assets,
								  memory_arena_handle_t *memory_arena);
void game_play_destroy(struct game_state *game_state);
b32 game_play_update(struct game_state *game_state);

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

INLINE void game_input_charge_reset(struct human_player *human_player)
{
	human_player->controls.charge.ready = human_player->controls.charge.charging = false;
}

INLINE void game_input_player_charge_reset(struct game_state *game_state,
										   entity_id_t player_entity_id)
{
	for (usize i = 0; i < MAX_INPUT_DEVICES; ++i) {
		if (game_state->human_players[i].controlled_player_entity_id == player_entity_id) {
			game_input_charge_reset(&game_state->human_players[i]);
			break;
		}
	}
}

#endif // !GAME_H
