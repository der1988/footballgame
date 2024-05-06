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
    CT_COUNT,
    CT_NONE
} ComponentType;

typedef struct ComponentTransform {
    struct ComponentTransform *parent;
    Vector2 position;
    f32 rotation;
    Vector2 scale;
    f32 virtual_height;
} ComponentTransform;

typedef struct {
    f32 mass;
    Vector2 accel;
    Vector2 velocity;
    f32 max_velocity;
    f32 max_force;
} ComponentPhysics;

typedef struct {
    f32 mass;
    Vector3 accel;
    Vector3 velocity;
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
} ColliderCircle;

typedef struct {
    v2 size;
    v2 offset;
} ColliderRectangle;

typedef union {
    ColliderCircle circle;
    ColliderRectangle rectangle;
} ColliderProperties;

typedef struct {
    ColliderShape shape;
    ColliderProperties props;
} ComponentCollider;

typedef struct {
    CameraType camera;
    f32 slowing_distance;
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
    b32 is_controlling_the_ball;
    AssetsSpriteFrameId stand_frames[D_COUNT];
    AssetsAnimationId run_animations[D_COUNT];
    Direction last_direction;
    b32 sprint_locked;
    f32 last_shot;
    f32 last_frame_update_time;
    u16 last_running_frame_idx;
} ComponentPlayer;

typedef struct {
    struct StateMachineBall *state_machine;
    EntityId last_player_on_the_ball_entity_id;
    EntityId last_kicker_entity_id;
    b32 subject_to_after_touch;
    f64 kick_time;
    AssetsSpriteFrameId roll_animations[D_COUNT];
    Direction last_direction;
    f32 last_frame_update_time;
    u16 last_running_frame_idx;
} ComponentBall;

typedef struct {
    Time hit_left_post_time;
    Time hit_right_post_time;
    Time hit_crossbar_time;
    b32 scored;
} ComponentGoal;

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
    };
} Component;

RunningActionIdx components_action_run(EntityId id, struct Action *action, GameState *game_state);
void components_action_stop(EntityId id, RunningActionIdx running_action_idx,
                            GameState *game_state);

void components_transform_calculate_absolute(ComponentTransform *transform, Vector2 *position,
                                             f32 *rotation, Vector2 *scale);

#endif // !COMPONENTS
