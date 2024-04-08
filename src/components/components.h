#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "../actions/actions.h"
#include "../assets/assets.h"
#include "../game.h"
#include "../sprites/sprites.h"
#include "types.h"
#include <raylib.h>

#define MaxConcurrentActions 5

struct Entity;

typedef enum {
    CT_TRANSFORM,
    CT_PHYSICS,
    CT_PHYSICS_3D,
    CT_SPRITE,
    CT_SHADOW,
    CT_COLLIDER,
    CT_STATE_MACHINE,
    CT_ACTION,
    CT_TARGET,
    CT_CAMERA_PROPS,
    CT_PLAYER,
    CT_BALL,
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
} ComponentSprite;

typedef struct {
    Color color;
    v2 offset;
    v2 scale;
} ComponentShadow;

typedef struct {
    void *ptr;
} ComponentStateMachine;

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
} ComponentCameraProps;

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
    b32 is_controlling_the_ball;
    AssetsSpriteFrameId stand_frames[D_COUNT];
    AssetsAnimationId run_animations[D_COUNT];
    Direction last_direction;
    f32 last_shot;
    f32 last_frame_update_time;
    u16 last_running_frame_idx;
} ComponentPlayer;

typedef struct {
    b32 subject_to_after_touch;
    f64 shot_time;
    AssetsSpriteFrameId roll_frames[D_COUNT];
} ComponentBall;

typedef struct {
    ComponentType type;
    b32 enabled;
    EntityId entity_id;
} ComponentHeader;

typedef struct {
    ComponentHeader header;
    union {
        ComponentCameraProps camera_props;
        ComponentTransform transform;
        ComponentPhysics physics;
        ComponentPhysics3d physics_3d;
        ComponentSprite sprite;
        ComponentShadow shadow;
        ComponentCollider collider;
        ComponentStateMachine state_machine;
        ComponentAction action;
        ComponentTarget target;
        ComponentPlayer player;
        ComponentBall ball;
    };
} Component;

RunningActionIdx components_action_run(EntityId id, struct Action *action, GameState *game_state);
void components_action_stop(EntityId id, RunningActionIdx running_action_idx,
                            GameState *game_state);

void components_transform_calculate_absolute(ComponentTransform *transform, Vector2 *position,
                                             f32 *rotation, Vector2 *scale);

#endif // !COMPONENTS
