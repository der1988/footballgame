#ifndef ACTIONS_H
#define ACTIONS_H

#include "assets.h"
#include "game.h"
#include "types.h"

enum ActionType
{
    AT_CHANGE_COLOR_FROM_TO,
    AT_PLAY_ANIMATION,
    AT_SEQUENCE,
    AT_SPAWN,
    AT_TWEEN_FLOAT_FROM_TO,
    AT_ROTATE_TO,
    AT_TOGGLE_BOOLEAN,
    AT_COUNT
};
static const char *ActionTypeToString[AT_COUNT] = {
    "change_color_from_to", "play_animation", "sequence",      "spawn",
    "tween_float_from_to",  "rotate_to",      "toggle_boolean"};

// Wrapper around an action to keep track of its state while being run
struct ActionRunner
{
    struct Action *action;
    b32 started;
    f32 start_time;
    b32 completed;
    b32 paused;
};

// Defines of an action
struct Action
{
    enum ActionType type;
    f32 duration;
    b32 loop;
    void (*start)(struct ActionRunner *, entity_id_t, struct GameState *game_state);
    void (*update)(struct ActionRunner *, entity_id_t, struct GameState *game_state);
};

// Changes the sprite color from one color to another
struct ActionChangeColorFromTo
{
    struct Action base;
    Color from;
    Color to;
};

// Plays an animation at a specific speed
struct ActionPlayAnimation
{
    struct Action base;
    AssetsAnimationId animation_id;
    f32 speed;
};

// Concatenates multiple actions to run them in series
struct ActionSequence
{
    struct Action base;
    u16 count;
    struct ActionRunner actions[];
};

// Spawn multiple actions to run them in parallel
struct ActionSpawn
{
    struct Action base;
    u16 count;
    struct ActionRunner actions[];
};

// Tween a float from one value to another
struct ActionTweenFloatFromTo
{
    struct Action base;
    f32 *value;
    f32 from;
    f32 to;
};

// Rotate a float to an angle
struct ActionRotateTo
{
    struct Action base;
    f32 from;
    f32 to;
    f32 diff;
};

// Rotate an aircraft to match its velocity direction
struct ActionAircraftRotateToVelocity
{
    struct Action base;
};

// Toggle boolean
struct ActionToggleBoolean
{
    struct Action base;
    b32 *value;
};

void actions_action_run(struct ActionRunner *action_runner, entity_id_t id,
			struct GameState *game_state);
void actions_action_stop(running_action_idx_t running_action_idx, entity_id_t id,
			 struct GameState *game_state);

struct ActionChangeColorFromTo *
actions_change_color_from_to_create(f32 duration, b32 loop, Color from, Color to,
				    memory_arena_handle_t *memory_arena_handle);
void actions_change_color_from_to_update(struct ActionRunner *action_runner, entity_id_t id,
					 struct GameState *game_state);

struct ActionPlayAnimation *
actions_play_animation_create(struct AssetsAnimation *animation, b32 loop, float speed,
			      memory_arena_handle_t *memory_arena_handle);
void actions_play_animation_update(struct ActionRunner *action_runner, entity_id_t id,
				   struct GameState *game_state);

struct ActionSequence *actions_sequence_create(memory_arena_handle_t *memory_arena_handle,
					       u32 count, ...);
void actions_sequence_update(struct ActionRunner *action_runner, entity_id_t id,
			     struct GameState *game_state);

struct ActionSpawn *actions_spawn_create(memory_arena_handle_t *memory_arena_handle, u32 count,
					 ...);
void actions_spawn_update(struct ActionRunner *action_runner, entity_id_t id,
			  struct GameState *game_state);

struct ActionTweenFloatFromTo *
actions_tween_float_from_to_create(f32 duration, b32 loop, f32 *value, f32 from, f32 to,
				   memory_arena_handle_t *memory_arena_handle);
void actions_tween_float_from_to_update(struct ActionRunner *action_runner, entity_id_t id,
					struct GameState *game_state);

struct ActionRotateTo *actions_rotate_to_create(f32 duration, b32 loop, f32 to,
						memory_arena_handle_t *memory_arena_handle);
void actions_rotate_to_update(struct ActionRunner *action_runner, entity_id_t id,
			      struct GameState *game_state);

void actions_toggle_boolean_update(struct ActionRunner *action_runner, entity_id_t id,
				   struct GameState *game_state);
struct ActionToggleBoolean *
actions_toggle_boolean_create(f32 duration, b32 loop, b32 *value,
			      memory_arena_handle_t *memory_arena_handle);
#endif // !ACTIONS_H
