#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "actions.h"
#include "assets.h"
#include "game.h"
#include "render.h"
#include "types.h"
#include <raylib.h>

#define MaxConcurrentActions 5

// Forward declarations
struct StateMachineBall;
struct StateMachinePlayer;

typedef struct {
    b32 enabled;
} ComponentHeader;

typedef struct ComponentTransform {
    ComponentHeader header;
    struct ComponentTransform *parent;
    v2 position;
    f32 rotation;
    v2 scale;
    f32 virtual_height;
} ComponentTransform;

typedef struct ComponentPhysics {
    ComponentHeader header;
    f32 mass;
    v2 accel;
    v2 velocity;
    f32 max_velocity;
    f32 max_force;
} ComponentPhysics;

typedef struct ComponentPhysics3d {
    ComponentHeader header;
    f32 mass;
    v3 accel;
    v3 velocity;
    f32 max_velocity;
    f32 max_force;
} ComponentPhysics3d;

typedef struct ComponentSprite {
    ComponentHeader header;
    Color color;
    b32 flipX;
    b32 flipY;
    SpriteLayer layer;
    i16 order_in_layer;
    v2 pivot_override;
    AssetsSpriteFrameId frame_id;
    b32 draw_shadow;
    Color shadow_color;
    v2 shadow_offset;
    v2 shadow_scale;
} ComponentSprite;

typedef struct ComponentAction {
    ComponentHeader header;
    ActionRunner running_actions[MaxConcurrentActions];
} ComponentAction;

typedef struct ComponentTarget {
    ComponentHeader header;
    EntityId entity_id;
} ComponentTarget;

typedef struct {
    enum {
        CS_CIRCLE,
        CS_RECTANGLE
    } shape;
    union {
        struct {
            f32 radius;
            v2 offset;
        } circle;
        struct {
            v2 size;
            v2 offset;
        } rectangle;
    } props;
} Collider;

typedef struct ComponentCollider {
    ComponentHeader header;
    Collider *colliders;
    u16 num_colliders;
} ComponentCollider;

typedef struct ComponentCamera {
    ComponentHeader header;
    CameraType camera;
    f32 slowing_distance;
    v2 offset;
} ComponentCamera;

typedef enum Direction {
    D_UP,
    D_DOWN,
    D_LEFT,
    D_RIGHT,
    D_UP_LEFT,
    D_UP_RIGHT,
    D_DOWN_LEFT,
    D_DOWN_RIGHT,
    D_COUNT
} Direction;

typedef struct ComponentPlayer {
    ComponentHeader header;
    PitchSide side;
    struct StateMachinePlayer *state_machine;
    b32 control_ball;
    Time time_spent_in_precision_control;
    Time time_spent_in_run_control;
    f32 current_max_velocity;
    AssetsSpriteFrameId stand_frames[D_COUNT];
    AssetsAnimationId run_animations[D_COUNT];
    Direction last_direction;
    f32 last_kick_time;
    f32 last_frame_update_time;
    u16 last_running_frame_idx;
    u8 team_formation_position_idx;
} ComponentPlayer;

typedef struct ComponentBall {
    ComponentHeader header;
    struct StateMachineBall *state_machine;
    b32 is_on_the_ground;
    EntityId last_player_on_the_ball_entity_id;
    EntityId last_kicker_entity_id;
    EntityId pass_target_entity_id;
    b32 subject_to_after_touch;
    v2 selected_after_touch_side;
    Time last_kick_time;
    Time last_bounce_time;
    AssetsSpriteFrameId roll_animations[D_COUNT];
    Direction last_direction;
    v2 future_bounce_position;
    u32 dts_to_future_bounce;
    v3 lookahead_positions[10];
    Time last_frame_update_time;
    u16 last_running_frame_idx;
} ComponentBall;

typedef struct ComponentGoal {
    ComponentHeader header;
    Time hit_left_post_time;
    Time hit_right_post_time;
    Time hit_crossbar_time;
    b32 scored;
} ComponentGoal;

typedef enum SaveSide {
    SS_UP,
    SS_DOWN,
    SS_COUNT
} SaveSide;

typedef enum SaveDirection {
    SD_LEFT,
    SD_RIGHT,
    SD_COUNT
} SaveDirection;

typedef enum SaveHeight {
    SH_HIGH,
    SH_LOW,
    SH_COUNT
} SaveHeight;

typedef enum SaveType {
    ST_WITHOUT_BALL,
    ST_WITH_BALL,
    ST_COUNT
} SaveType;

typedef enum AnimationPhase {
    AP_START,
    AP_END,
    AP_COUNT
} AnimationPhase;

typedef struct ComponentGoalie {
    ComponentHeader header;
    struct StateMachineGoalie *state_machine;
    SaveDirection dive_direction;
    SaveHeight dive_height;
    f32 dive_distance;
    f32 dive_speed;
    AssetsSpriteFrameId save_frames[SS_COUNT][ST_COUNT];
    AssetsSpriteFrameId dash_frames[SS_COUNT][SD_COUNT][ST_COUNT];
    AssetsAnimationId dive_animations_start[SS_COUNT][SD_COUNT][SH_COUNT];
    AssetsAnimationId dive_animations_end[SS_COUNT][SD_COUNT][SH_COUNT][ST_COUNT];
    Time last_frame_update_time;
    Time last_save_time;
} ComponentGoalie;

RunningActionIdx components_action_run(EntityId id, struct Action *action, GameState *game_state);
void components_action_stop(EntityId id, RunningActionIdx running_action_idx,
                            GameState *game_state);

void components_transform_calculate_absolute(ComponentTransform *transform, Vector2 *position,
                                             f32 *rotation, Vector2 *scale);

#endif // !COMPONENTS
