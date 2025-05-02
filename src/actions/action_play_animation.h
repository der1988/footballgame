#ifndef ACTION_PLAY_ANIMATION_H
#define ACTION_PLAY_ANIMATION_H

#include "action.h"
#include "asset_animation.h"

// Plays an animation at a specific speed
struct action_play_animation {
	struct action base;
	asset_animation_id_t animation_id;
	f32 speed;
};

struct action_play_animation *
action_play_animation_create(struct asset_animation *animation,
							 b32 loop,
							 float speed,
							 memory_arena_handle_t *memory_arena_handle);
void action_play_animation_update(struct action_runner *action_runner,
								  entity_id_t id,
								  struct game_state *game_state);

#endif // !ACTION_PLAY_ANIMATION_H
