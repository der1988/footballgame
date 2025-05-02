#include "action_play_animation.h"
#include "entities.h"
#include "game.h"
#include <math.h>

void action_play_animation_update(struct action_runner *action_runner,
								  entity_id_t id,
								  struct game_state *game_state)
{
	struct action_play_animation *action = (struct action_play_animation *)action_runner->action;
	f32 h = (game_state->time - action_runner->start_time) / action_runner->action->duration;
	if (h >= 1) {
		h = 1;
		action_runner->completed = true;
	}
	struct component_sprite *sprite = &entity_get(game_state->entity_repository, id)->sprite;
	struct assets_animation *animation =
		assets_animation_get_by_id(game_state->assets.animation_cache, action->animation_id);
	u16 frame_to_show_idx = floorf(h * (animation->frames_count - 1));
	assets_sprite_frame_id_t frame_id = animation->frames[frame_to_show_idx];
	sprite->frame_id = frame_id;
}

struct action_play_animation *
action_play_animation_create(struct assets_animation *animation,
							 b32 loop,
							 float speed,
							 memory_arena_handle_t *memory_arena_handle)
{
	struct action_play_animation *action = memory_arena_push(memory_arena_handle, sizeof(*action));
	*action = (struct action_play_animation){
		.base = (struct action){.type = AT_PLAY_ANIMATION,
								.duration = (animation->delay * animation->frames_count / speed),
								.loop = loop,
								.update = action_play_animation_update},
		.animation_id = animation->id,
		.speed = speed};
	return action;
}
