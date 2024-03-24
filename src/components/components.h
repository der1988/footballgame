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
    CT_SPRITE,
    CT_STATE_MACHINE,
    CT_ACTION,
    CT_TARGET,
    CT_CAMERA_PROPS,
    CT_HUMAN,
    CT_COUNT,
    CT_NONE
} ComponentType;

typedef struct ComponentTransform {
    struct ComponentTransform *parent;
    Vector2 position;
    float rotation;
    Vector2 scale;
} ComponentTransform;

typedef struct {
    f32 mass;
    Vector2 accel;
    Vector2 velocity;
} ComponentPhysics;

typedef struct {
    Color color;
    b32 flipX;
    b32 flipY;
    SpriteLayer layer;
    i16 order_in_layer;
    AssetsSpriteFrameId frame_id;
} ComponentSprite;

typedef struct {
    void *ptr;
} ComponentStateMachine;

typedef struct {
    ActionRunner running_actions[MaxConcurrentActions];
} ComponentAction;

typedef struct {
    ComponentTransform *target_transform;
} ComponentTarget;

typedef struct {
    CameraType camera;
} ComponentCameraProps;

typedef enum {
    PD_UP,
    PD_DOWN,
    PD_LEFT,
    PD_RIGHT,
    PD_UP_LEFT,
    PD_UP_RIGHT,
    PD_DOWN_LEFT,
    PD_DOWN_RIGHT,
    PD_COUNT
} PlayerDirection;

typedef struct {
    f32 max_velocity;
    AssetsSpriteFrameId stand_frames[PD_COUNT];
    struct Action *run_actions[PD_COUNT];
} ComponentHuman;

typedef struct {
    ComponentType type;
    b32 enabled;
    EntityId entity_id;
} ComponentHeader;

typedef struct {
    ComponentHeader header;
    union {
        ComponentTransform transform;
        ComponentPhysics physics;
        ComponentSprite sprite;
        ComponentStateMachine state_machine;
        ComponentAction action;
        ComponentTarget target;
        ComponentHuman human;
        ComponentCameraProps camera_props;
    };
} Component;

RunningActionIdx components_action_run(EntityId id, struct Action *action, GameState *game_state);
void components_action_stop(EntityId id, RunningActionIdx running_action_idx,
                            GameState *game_state);

void components_transform_calculate_absolute(ComponentTransform *transform, Vector2 *position,
                                             f32 *rotation, Vector2 *scale);

#endif // !COMPONENTS
