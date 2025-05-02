#ifndef ACTION_PLAY_ANIMATION_H
#define ACTION_PLAY_ANIMATION_H

#include "action.h"
#include "assets.h"

// Plays an animation at a specific speed
struct action_play_animation {
	struct action base;
	assets_animation_id_t animation_id;
	f32 speed;
};

struct action_play_animation *
actions_play_animation_create(struct assets_animation *animation,
							  b32 loop,
							  float speed,
							  memory_arena_handle_t *memory_arena_handle);
void actions_play_animation_update(struct action_runner *action_runner,
								   entity_id_t id,
								   struct game_state *game_state);

#endif // !ACTION_PLAY_ANIMATION_H
