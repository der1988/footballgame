#ifndef GAME_H
#define GAME_H

#include "assets.h"
#include "football.h"
#include "memory.h"
#include "raygui/raygui.h"
#include "types.h"
#include <raylib.h>
#include "layers.h"
#include "tweakui.h"

// Forward declarations
struct entity_repository;
struct input;

enum memory_arena_lifetime {
    MAL_LEVEL,
    MAL_UPDATE,
    MAL_DRAW,
    MAL_COUNT
};

enum camera_type {
    CAM_MAIN,
    CAM_COUNT
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
    f32 ball_after_touch_delay_time;
    f32 ball_after_touch_max_time;
    f32 ball_after_touch_force;
    f32 ball_max_velocity;
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

struct controls {
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
    struct controls controls;
    entity_id_t player_entity_id;
};

struct player_on_the_ball {
    entity_id_t id;
    f32 distance_from_the_ball;
    b32 kicked_ball;
    struct player_controls {
	v2 steering;
	f32 shot;
	f32 pass;
	b32 automatic_pass;
	f32 long_pass;
    } controls;
};

struct game_vars {
    struct match match;
    struct pitch pitch;
    struct player_on_the_ball player_on_the_ball;
    entity_id_t camera_entity_id;
    entity_id_t ball_entity_id;
    struct human_player p1;
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
    memory_arena_handle_t *memory_by_lifetime[MAL_COUNT];
    struct assets *assets;
    struct entity_repository *entity_repository;
    tweakui_t *tweakui;
    layers_debug_t *layer_debug;
    assets_texture_id_t pitch_texture_id;
    render_texture_t render_texture;
    camera2d_t camera;
    camera2d_t virtual_camera;
};

struct game_state *game_play_init(struct game_config gameConfig,
				  struct assets *assets,
				  memory_arena_handle_t *memory_arena);
void game_play_destroy(struct game_state *game_state);
void game_play_update(struct game_state *game_state, struct input *input);
void game_play_draw(struct game_state *game_state);

void game_loop(struct game_config config,
	       struct assets *assets,
	       memory_arena_handle_t *memory_arena);

#endif // !GAME_H
