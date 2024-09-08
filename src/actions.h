#ifndef ACTIONS_H
#define ACTIONS_H

#include "assets.h"
#include "game.h"
#include "types.h"

typedef enum {
    AT_CHANGE_COLOR_FROM_TO,
    AT_PLAY_ANIMATION,
    AT_SEQUENCE,
    AT_SPAWN,
    AT_TWEEN_FLOAT_FROM_TO,
    AT_ROTATE_TO,
    AT_TOGGLE_BOOLEAN,
    AT_COUNT
} ActionType;
static const char *ActionTypeToString[AT_COUNT] = {
    "change_color_from_to", "play_animation", "sequence",      "spawn",
    "tween_float_from_to",  "rotate_to",      "toggle_boolean"};

// Wrapper around an action to keep track of its state while being run
typedef struct {
    struct Action *action;
    b32 started;
    f32 start_time;
    b32 completed;
    b32 paused;
} ActionRunner;

// Defines of an action
struct Action {
    ActionType type;
    f32 duration;
    b32 loop;
    void (*start)(ActionRunner *, EntityId, GameState *game_state);
    void (*update)(ActionRunner *, EntityId, GameState *game_state);
};

// Changes the sprite color from one color to another
typedef struct {
    struct Action base;
    Color from;
    Color to;
} ActionChangeColorFromTo;

// Plays an animation at a specific speed
typedef struct {
    struct Action base;
    AssetsAnimationId animation_id;
    f32 speed;
} ActionPlayAnimation;

// Concatenates multiple actions to run them in series
typedef struct {
    struct Action base;
    u16 count;
    ActionRunner actions[];
} ActionSequence;

// Spawn multiple actions to run them in parallel
typedef struct {
    struct Action base;
    u16 count;
    ActionRunner actions[];
} ActionSpawn;

// Tween a float from one value to another
typedef struct {
    struct Action base;
    f32 *value;
    f32 from;
    f32 to;
} ActionTweenFloatFromTo;

// Rotate a float to an angle
typedef struct {
    struct Action base;
    f32 from;
    f32 to;
    f32 diff;
} ActionRotateTo;

// Rotate an aircraft to match its velocity direction
typedef struct {
    struct Action base;
} ActionAircraftRotateToVelocity;

// Toggle boolean
typedef struct {
    struct Action base;
    b32 *value;
} ActionToggleBoolean;

void actions_action_run(ActionRunner *action_runner, EntityId id, GameState *game_state);
void actions_action_stop(RunningActionIdx running_action_idx, EntityId id, GameState *game_state);

ActionChangeColorFromTo *
actions_change_color_from_to_create(f32 duration, b32 loop, Color from, Color to,
                                    MemoryArenaHandle *memory_arena_handle);
void actions_change_color_from_to_update(ActionRunner *action_runner, EntityId id,
                                         GameState *game_state);

ActionPlayAnimation *actions_play_animation_create(AssetsAnimation *animation, b32 loop,
                                                   float speed,
                                                   MemoryArenaHandle *memory_arena_handle);
void actions_play_animation_update(ActionRunner *action_runner, EntityId id, GameState *game_state);

ActionSequence *actions_sequence_create(MemoryArenaHandle *memory_arena_handle, u32 count, ...);
void actions_sequence_update(ActionRunner *action_runner, EntityId id, GameState *game_state);

ActionSpawn *actions_spawn_create(MemoryArenaHandle *memory_arena_handle, u32 count, ...);
void actions_spawn_update(ActionRunner *action_runner, EntityId id, GameState *game_state);

ActionTweenFloatFromTo *actions_tween_float_from_to_create(f32 duration, b32 loop, f32 *value,
                                                           f32 from, f32 to,
                                                           MemoryArenaHandle *memory_arena_handle);
void actions_tween_float_from_to_update(ActionRunner *action_runner, EntityId id,
                                        GameState *game_state);

ActionRotateTo *actions_rotate_to_create(f32 duration, b32 loop, f32 to,
                                         MemoryArenaHandle *memory_arena_handle);
void actions_rotate_to_update(ActionRunner *action_runner, EntityId id, GameState *game_state);

void actions_toggle_boolean_update(ActionRunner *action_runner, EntityId id, GameState *game_state);
ActionToggleBoolean *actions_toggle_boolean_create(f32 duration, b32 loop, b32 *value,
                                                   MemoryArenaHandle *memory_arena_handle);
#endif // !ACTIONS_H
