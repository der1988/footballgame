#ifndef ACTIONS_H
#define ACTIONS_H

#include "assets.h"
#include "game.h"
#include "types.h"

enum action_type
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
struct action_runner
{
    struct action *action;
    b32 started;
    f32 start_time;
    b32 completed;
    b32 paused;
};

// Defines of an action
struct action
{
    enum action_type type;
    f32 duration;
    b32 loop;
    void (*start)(struct action_runner *, entity_id_t, struct game_state *game_state);
    void (*update)(struct action_runner *, entity_id_t, struct game_state *game_state);
};

// Changes the sprite color from one color to another
struct action_change_color_from_to
{
    struct action base;
    Color from;
    Color to;
};

// Plays an animation at a specific speed
struct action_play_animation
{
    struct action base;
    assets_animation_id_t animation_id;
    f32 speed;
};

// Concatenates multiple actions to run them in series
struct action_sequence
{
    struct action base;
    u16 count;
    struct action_runner actions[];
};

// Spawn multiple actions to run them in parallel
struct action_spawn
{
    struct action base;
    u16 count;
    struct action_runner actions[];
};

// Tween a float from one value to another
struct action_tween_float_from_to
{
    struct action base;
    f32 *value;
    f32 from;
    f32 to;
};

// Rotate a float to an angle
struct action_rotate_to
{
    struct action base;
    f32 from;
    f32 to;
    f32 diff;
};

// Toggle boolean
struct action_toggle_boolean
{
    struct action base;
    b32 *value;
};

void actions_action_run(struct action_runner *action_runner, entity_id_t id,
			struct game_state *game_state);
void actions_action_stop(running_action_idx_t running_action_idx, entity_id_t id,
			 struct game_state *game_state);

struct action_change_color_from_to *
actions_change_color_from_to_create(f32 duration, b32 loop, Color from, Color to,
				    memory_arena_handle_t *memory_arena_handle);
void actions_change_color_from_to_update(struct action_runner *action_runner, entity_id_t id,
					 struct game_state *game_state);

struct action_play_animation *
actions_play_animation_create(struct assets_animation *animation, b32 loop, float speed,
			      memory_arena_handle_t *memory_arena_handle);
void actions_play_animation_update(struct action_runner *action_runner, entity_id_t id,
				   struct game_state *game_state);

struct action_sequence *actions_sequence_create(memory_arena_handle_t *memory_arena_handle,
						u32 count, ...);
void actions_sequence_update(struct action_runner *action_runner, entity_id_t id,
			     struct game_state *game_state);

struct action_spawn *actions_spawn_create(memory_arena_handle_t *memory_arena_handle, u32 count,
					  ...);
void actions_spawn_update(struct action_runner *action_runner, entity_id_t id,
			  struct game_state *game_state);

struct action_tween_float_from_to *
actions_tween_float_from_to_create(f32 duration, b32 loop, f32 *value, f32 from, f32 to,
				   memory_arena_handle_t *memory_arena_handle);
void actions_tween_float_from_to_update(struct action_runner *action_runner, entity_id_t id,
					struct game_state *game_state);

struct action_rotate_to *actions_rotate_to_create(f32 duration, b32 loop, f32 to,
						  memory_arena_handle_t *memory_arena_handle);
void actions_rotate_to_update(struct action_runner *action_runner, entity_id_t id,
			      struct game_state *game_state);

void actions_toggle_boolean_update(struct action_runner *action_runner, entity_id_t id,
				   struct game_state *game_state);
struct action_toggle_boolean *
actions_toggle_boolean_create(f32 duration, b32 loop, b32 *value,
			      memory_arena_handle_t *memory_arena_handle);
#endif // !ACTIONS_H
