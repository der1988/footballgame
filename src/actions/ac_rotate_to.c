#include "actions.h"
#include "components.h"
#include "entities.h"
#include <math.h>
#include <raylib.h>

void actions_rotate_to_update(struct ActionRunner *action_runner, entity_id_t id,
			      struct GameState *game_state)
{
    struct ActionRotateTo *action = (struct ActionRotateTo *)action_runner->action;

    struct Entity *entity = entities_entity_get(game_state->entity_repository, id);

    f32 h = (game_state->time - action_runner->start_time) / action_runner->action->duration;
    if (h >= 1)
    {
	h = 1;
	action_runner->completed = true;
    }

    entity->transform.rotation = action->from + action->diff * h;
}

void actions_rotate_to_start(struct ActionRunner *action_runner, entity_id_t id,
			     struct GameState *game_state)
{
    struct ActionRotateTo *action = (struct ActionRotateTo *)action_runner->action;

    struct Entity *entity = entities_entity_get(game_state->entity_repository, id);

    f32 start_angle = entity->transform.rotation;
    if (start_angle > 0)
	start_angle = fmodf(start_angle, 360.0f);
    else
	start_angle = fmodf(start_angle, -360.0f);

    f32 diff_angle = action->to - start_angle;
    if (diff_angle > 180)
	diff_angle -= 360;
    if (diff_angle < -180)
	diff_angle += 360;

    // These need to be calculated at every start
    action->from = entity->transform.rotation;
    action->diff = diff_angle;
}

struct ActionRotateTo *actions_rotate_to_create(f32 duration, b32 loop, f32 to,
						MemoryArenaHandle *memory_arena_handle)
{
    struct ActionRotateTo *action = memory_arena_push(memory_arena_handle, sizeof(*action));

    *action = (struct ActionRotateTo){.base = (struct Action){.type = AT_ROTATE_TO,
							      .duration = duration,
							      .loop = loop,
							      .start = actions_rotate_to_start,
							      .update = actions_rotate_to_update},
				      .to = to};
    return action;
}
