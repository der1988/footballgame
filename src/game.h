#ifndef GAME_H
#define GAME_H

#include "assets/assets.h"
#include "raygui/raygui.h"
#include "types.h"
#include <raylib.h>

// Forward declarations
struct EntityRepository;
struct AssetsTextureCache;
struct AssetsSpriteFrameCache;
struct AssetsAnimationCache;
struct TweakUI;
struct LayersDebug;
struct Input;

typedef enum { MAL_GAME, MAL_LEVEL, MAL_FRAME, MAL_COUNT } MemoryArenaLifetime;

typedef enum { CAM_MAIN, CAM_PLAYER_2, CAM_COUNT } CameraType;

typedef struct GameConfig {
    u32 screen_width;
    u32 screen_height;
    void *memory;
    u32 players_count;
} GameConfig;

typedef struct GameConstants {
    b32 render_debug;
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
    f32 player_movement_force;
    f32 player_movement_drag;
    f32 player_control_max_velocity;
    f32 player_run_max_velocity;
    f32 player_control_radius;
} GameConstants;

typedef enum ActionChargeType { ACT_SHOOT, ACT_PASS, ACT_LONG_PASS } ActionChargeType;

typedef struct ActionCharge {
    ActionChargeType type;
    v2 normalised_direction;
    Time started;
    Time ended;
    f32 power;
    b32 charging;
    b32 ready;
} ActionCharge;

typedef struct Controls {
    v2 normalised_direction;
    f32 sprint;
    ActionCharge charge;
} Controls;

typedef struct HumanPlayer {
    Controls controls;
    EntityId player_entity_id;
} HumanPlayer;

typedef struct GameVariables {
    b32 is_tweakui_shown;
    EntityId ball_entity_id;
    HumanPlayer p1;
    Rectangle pitch_rectangle;
    b32 ball_is_playable;
    b32 someone_scored;
} GameVariables;

typedef struct GameState {
    b32 initialised;
    GameConfig config;
    GameConstants constants;
    GameVariables vars;
    MemoryArenaHandle *memory_by_lifetime[MAL_COUNT];
    struct EntityRepository *entity_repository;
    struct AssetsTextureCache *texture_cache;
    struct AssetsSpriteFrameCache *sprite_frame_cache;
    struct AssetsAnimationCache *animation_cache;
    struct TweakUI *tweakui;
    struct LayersDebug *layer_debug;
    AssetsTextureId pitch_texture_id;
    RenderTexture2D render_texture;
    Camera2D camera;
    Time time;
    DeltaTime dt;
} GameState;

GameState *game_init(GameConfig gameConfig);
void game_destroy(GameState *game_state);
void game_update(GameState *game_state, struct Input *input);
void game_draw(GameState *game_state);

#endif // !GAME_H
