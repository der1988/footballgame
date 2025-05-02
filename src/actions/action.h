#ifndef ACTION_H
#define ACTION_H

#include "types.h"

// Forward declaration
struct game_state;

enum action_type {
	AT_CHANGE_COLOR_FROM_TO,
	AT_PLAY_ANIMATION,
	AT_SEQUENCE,
	AT_SPAWN,
	AT_TWEEN_FLOAT_FROM_TO,
	AT_ROTATE_TO,
	AT_TOGGLE_BOOLEAN,
	AT_COUNT
};
static const char *ActionTypeToString[AT_COUNT] = {"change_color_from_to",
												   "play_animation",
												   "sequence",
												   "spawn",
												   "tween_float_from_to",
												   "rotate_to",
												   "toggle_boolean"};

// Wrapper around an action to keep track of its state while being run
struct action_runner {
	struct action *action;
	b32 started;
	f32 start_time;
	b32 completed;
	b32 paused;
};

// Defines of an action
struct action {
	enum action_type type;
	f32 duration;
	b32 loop;
	void (*start)(struct action_runner *, entity_id_t, struct game_state *game_state);
	void (*update)(struct action_runner *, entity_id_t, struct game_state *game_state);
};

void action_run(struct action_runner *action_runner, entity_id_t id, struct game_state *game_state);
void action_stop(running_action_idx_t running_action_idx,
				 entity_id_t id,
				 struct game_state *game_state);

#endif // !ACTION_H
