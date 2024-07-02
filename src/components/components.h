#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "../actions/actions.h"
#include "../assets/assets.h"
#include "../game.h"
#include "../sprites/sprites.h"
#include "types.h"
#include <raylib.h>

#define MaxConcurrentActions 5

// Forward declarations
struct StateMachineBall;
struct StateMachinePlayer;

typedef enum {
    CT_TRANSFORM,
    CT_PHYSICS,
    CT_PHYSICS_3D,
    CT_SPRITE,
    CT_COLLIDER,
    CT_ACTION,
    CT_TARGET,
    CT_CAMERA,
    CT_PLAYER,
    CT_BALL,
    CT_GOAL,
    CT_GOALIE,
    CT_COUNT,
    CT_NONE
} ComponentType;

typedef struct ComponentTransform {
    struct ComponentTransform *parent;
    v2 position;
    f32 rotation;
    v2 scale;
    f32 virtual_height;
} ComponentTransform;

typedef struct {
    f32 mass;
    v2 accel;
    v2 velocity;
    f32 max_velocity;
    f32 max_force;
} ComponentPhysics;

typedef struct {
    f32 mass;
    v3 accel;
    v3 velocity;
    f32 max_velocity;
    f32 max_force;
} ComponentPhysics3d;

typedef struct {
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

typedef struct {
    ActionRunner running_actions[MaxConcurrentActions];
} ComponentAction;

typedef struct {
    EntityId entity_id;
} ComponentTarget;

typedef enum { CS_CIRCLE, CS_RECTANGLE } ColliderShape;

typedef struct {
    f32 radius;
    v2 offset;
} ColliderPropsCircle;

typedef struct {
    v2 size;
    v2 offset;
} ColliderPropsRectangle;

typedef struct {
    ColliderShape shape;
    union {
        ColliderPropsCircle props_circle;
        ColliderPropsRectangle props_rectangle;
    };
} Collider;

typedef struct {
    Collider *colliders;
    u16 num_colliders;
} ComponentCollider;

typedef struct {
    CameraType camera;
    f32 slowing_distance;
    v2 offset;
} ComponentCamera;

typedef enum {
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

typedef struct {
    struct StateMachinePlayer *state_machine;
    b32 control_ball;
    Time time_spent_in_precision_control;
    Time time_spent_in_run_control;
    f32 current_max_velocity;
    AssetsSpriteFrameId stand_frames[D_COUNT];
    AssetsAnimationId run_animations[D_COUNT];
    Direction last_direction;
    f32 last_shot;
    f32 last_frame_update_time;
    u16 last_running_frame_idx;
} ComponentPlayer;

typedef struct {
    struct StateMachineBall *state_machine;
    b32 is_on_the_ground;
    EntityId last_player_on_the_ball_entity_id;
    EntityId last_kicker_entity_id;
    b32 subject_to_after_touch;
    f64 kick_time;
    AssetsSpriteFrameId roll_animations[D_COUNT];
    Direction last_direction;
    v2 future_bounce_position;
    v3 lookahead_positions[10];
    f32 last_frame_update_time;
    u16 last_running_frame_idx;
} ComponentBall;

typedef struct {
    Time hit_left_post_time;
    Time hit_right_post_time;
    Time hit_crossbar_time;
    b32 scored;
} ComponentGoal;

typedef enum { SS_UP, SS_DOWN, SS_COUNT } SaveSide;
typedef enum { SD_LEFT, SD_RIGHT, SD_COUNT } SaveDirection;
typedef enum { SH_HIGH, SH_LOW, SH_COUNT } SaveHeight;
typedef enum { ST_WITHOUT_BALL, ST_WITH_BALL, ST_COUNT } SaveType;
typedef enum { AP_START, AP_END, AP_COUNT } AnimationPhase;

typedef struct {
    struct StateMachineGoalie *state_machine;
    SaveDirection dive_direction;
    SaveHeight dive_height;
    f32 dive_distance;
    f32 dive_speed;
    AssetsSpriteFrameId save_frames[SS_COUNT][ST_COUNT];
    AssetsSpriteFrameId dash_frames[SS_COUNT][SD_COUNT][ST_COUNT];
    AssetsAnimationId dive_animations[SS_COUNT][SD_COUNT][SH_COUNT][AP_COUNT];
    f32 last_frame_update_time;
} ComponentGoalie;

typedef struct {
    ComponentType type;
    b32 enabled;
    EntityId entity_id;
} ComponentHeader;

typedef struct {
    ComponentHeader header;
    union {
        ComponentCamera camera;
        ComponentTransform transform;
        ComponentPhysics physics;
        ComponentPhysics3d physics_3d;
        ComponentSprite sprite;
        ComponentCollider collider;
        ComponentAction action;
        ComponentTarget target;
        ComponentPlayer player;
        ComponentBall ball;
        ComponentGoal goal;
        ComponentGoalie goalie;
    };
} Component;

RunningActionIdx components_action_run(EntityId id, struct Action *action, GameState *game_state);
void components_action_stop(EntityId id, RunningActionIdx running_action_idx,
                            GameState *game_state);

void components_transform_calculate_absolute(ComponentTransform *transform, Vector2 *position,
                                             f32 *rotation, Vector2 *scale);

#endif // !COMPONENTS
