#include "actions.h"
#include "macro.h"
#include <raylib.h>
#include <stdarg.h>

void actions_spawn_start(struct action_runner *action_runner, entity_id_t id,
			 struct game_state *game_state)
{
    UNUSED(id);
    UNUSED(game_state);

    struct action_spawn *action_spawn = (struct action_spawn *)action_runner->action;

    // Reset all children actions
    for (usize i = 0; i < action_spawn->count; ++i)
    {
	action_spawn->actions[i].started = false;
    }
}

void actions_spawn_update(struct action_runner *action_runner, entity_id_t id,
			  struct game_state *game_state)
{
    struct action_spawn *action_spawn = (struct action_spawn *)action_runner->action;

    usize i = 0;
    usize total_completed = 0;
    for (i = 0; i < action_spawn->count; ++i)
    {
	actions_action_run(&action_spawn->actions[i], id, game_state);
	if (action_spawn->actions[i].completed)
	{
	    total_completed++;
	}
    }

    if (total_completed == action_spawn->count)
    {
	action_runner->completed = true;
    }
}

struct action_spawn *actions_spawn_create(memory_arena_handle_t *memory_arena_handle, u32 count,
					  ...)
{
    struct action_spawn *action_spawn = memory_arena_push(
	memory_arena_handle, sizeof(struct action_spawn) + sizeof(struct action_runner) * count);
    *action_spawn = (struct action_spawn){
	.base = (struct action){.type = AT_SPAWN,
				.loop = false,
				.start = actions_spawn_start,
				.update = actions_spawn_update},
    };

    f32 max_duration = 0;
    va_list ap;
    va_start(ap, count);
    for (usize i = 0; i < count; ++i)
    {
	action_spawn->actions[i] = (struct action_runner){.action = va_arg(ap, struct Action *)};
	f32 tmp_duration = 0;
	if ((tmp_duration = action_spawn->actions[i].action->duration) > max_duration)
	    max_duration = tmp_duration;
    }
    va_end(ap);

    action_spawn->count = count;
    action_spawn->base.duration = max_duration;

    return action_spawn;
}
