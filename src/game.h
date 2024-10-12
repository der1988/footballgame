#ifndef GAME_H
#define GAME_H

#include "assets.h"
#include "football.h"
#include "raygui/raygui.h"
#include "types.h"
#include <raylib.h>
#include "layers.h"

// Forward declarations
struct EntityRepository;
struct AssetsTextureCache;
struct AssetsSpriteFrameCache;
struct AssetsAnimationCache;
struct TweakUI;
struct Input;

enum MemoryArenaLifetime
{
    MAL_GAME,
    MAL_LEVEL,
    MAL_UPDATE,
    MAL_DRAW,
    MAL_COUNT
};

enum CameraType
{
    CAM_MAIN,
    CAM_COUNT
};

struct GameConfig
{
    u32 virtual_screen_width;
    u32 virtual_screen_height;
    u32 screen_width;
    u32 screen_height;
    void *memory;
    u32 players_count;
};

struct GameConstants
{
    f64 fixed_timestep;
    b32 render_debug;
    b32 ai_enabled;
    b32 shoot_aid_enabled;
    b32 shoot_aid_no_inertia;
    f32 input_stick_dead_zone;
    v3 gravity_force;
    f32 pixels_per_meter;
    f32 air_density;
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
    f32 kick_charge_reset_time;
    u32 crossbar_height;
};

struct Controls
{
    v2 steering;
    struct ActionCharge
    {
	enum ActionChargeType
	{
	    ACT_SHOOT,
	    ACT_PASS,
	    ACT_LONG_PASS
	} type;
	v2 direction;
	time started;
	time ended;
	f32 power;
	b32 charging;
	b32 ready;
    } charge;
};

struct HumanPlayer
{
    struct Controls controls;
    EntityId player_entity_id;
};

struct GameVariables
{
    struct Match match;
    struct Pitch pitch;
    EntityId camera_entity_id;
    EntityId ball_entity_id;
    struct HumanPlayer p1;
    b32 slow_motion;
    b32 is_log_shown;
    b32 is_tweakui_shown;
};

struct GameState
{
    b32 initialised;
    struct GameConfig config;
    struct GameConstants constants;
    struct GameVariables vars;
    MemoryArenaHandle *memory_by_lifetime[MAL_COUNT];
    struct EntityRepository *entity_repository;
    struct AssetsTextureCache *texture_cache;
    struct AssetsSpriteFrameCache *sprite_frame_cache;
    struct AssetsAnimationCache *animation_cache;
    struct TweakUI *tweakui;
    LayersDebug *layer_debug;
    AssetsTextureId pitch_texture_id;
    RenderTexture2D render_texture;
    Camera2D camera;
    Camera2D virtual_camera;
    time time;
    DeltaTime dt;
};

struct GameState *game_init(struct GameConfig gameConfig);
void game_destroy(struct GameState *game_state);
void game_update(struct GameState *game_state, struct Input *input);
void game_draw(struct GameState *game_state);

#endif // !GAME_H
