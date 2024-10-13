#include "actions.h"
#include "assets.h"
#include "components.h"
#include "entities.h"
#include <math.h>
#include <raylib.h>

void actions_play_animation_update(struct ActionRunner *action_runner, entity_id_t id,
				   struct GameState *game_state)
{
    struct ActionPlayAnimation *action = (struct ActionPlayAnimation *)action_runner->action;
    f32 h = (game_state->time - action_runner->start_time) / action_runner->action->duration;
    if (h >= 1)
    {
	h = 1;
	action_runner->completed = true;
    }
    struct ComponentSprite *sprite =
	&entities_entity_get(game_state->entity_repository, id)->sprite;
    struct AssetsAnimation *animation =
	assets_animation_get_by_id(game_state->animation_cache, action->animation_id);
    u16 frame_to_show_idx = floorf(h * (animation->frames_count - 1));
    AssetsSpriteFrameId frame_id = animation->frames[frame_to_show_idx];
    sprite->frame_id = frame_id;
}

struct ActionPlayAnimation *
actions_play_animation_create(struct AssetsAnimation *animation, b32 loop, float speed,
			      memory_arena_handle_t *memory_arena_handle)
{
    struct ActionPlayAnimation *action = memory_arena_push(memory_arena_handle, sizeof(*action));
    *action = (struct ActionPlayAnimation){
	.base = (struct Action){.type = AT_PLAY_ANIMATION,
				.duration = (animation->delay * animation->frames_count / speed),
				.loop = loop,
				.update = actions_play_animation_update},
	.animation_id = animation->id,
	.speed = speed};
    return action;
}
