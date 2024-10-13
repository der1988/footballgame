#include "actions.h"
#include "entities.h"
#include "utils.h"
#include <raylib.h>

void actions_change_color_from_to_update(struct action_runner *action_runner, entity_id_t id,
					 struct game_state *game_state)
{
    struct action_change_color_from_to *action =
	(struct action_change_color_from_to *)action_runner->action;
    f32 h = (game_state->time - action_runner->start_time) / action_runner->action->duration;
    if (h >= 1)
    {
	h = 1;
	action_runner->completed = true;
    }
    struct component_sprite *sprite =
	&entities_entity_get(game_state->entity_repository, id)->sprite;
    sprite->color = utils_color_lerp(action->from, action->to, h);
}

struct action_change_color_from_to *
actions_change_color_from_to_create(f32 duration, b32 loop, Color from, Color to,
				    memory_arena_handle_t *memory_arena_handle)
{
    struct action_change_color_from_to *action =
	memory_arena_push(memory_arena_handle, sizeof(*action));
    *action = (struct action_change_color_from_to){
	.base = (struct action){.type = AT_CHANGE_COLOR_FROM_TO,
				.duration = duration,
				.loop = loop,
				.update = actions_change_color_from_to_update},
	.from = from,
	.to = to,
    };
    return action;
}
