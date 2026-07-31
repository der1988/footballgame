#include "action_rotate_to.h"
#include "game.h"
#include "entity.h"
#include <assert.h>
#include <math.h>

void action_rotate_to_update(struct action_runner *action_runner,
							 entity_id_t id,
							 struct game_state *game_state)
{
	struct action_rotate_to *action = (struct action_rotate_to *)action_runner->action;

	struct entity *entity = entity_get(game_state->entity_repository, id);

	f32 h = (game_state->time - action_runner->start_time) / action_runner->action->duration;
	if (h >= 1) {
		h = 1;
		action_runner->completed = true;
	}

	entity->transform.rotation = action->from + action->diff * h;
}

void action_rotate_to_start(struct action_runner *action_runner,
							entity_id_t id,
							struct game_state *game_state)
{
	struct action_rotate_to *action = (struct action_rotate_to *)action_runner->action;

	struct entity *entity = entity_get(game_state->entity_repository, id);

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

struct action_rotate_to *
action_rotate_to_create(f32 duration, b32 loop, f32 to, memory_arena_handle_t *memory_arena_handle)
{
	struct action_rotate_to *action = memory_arena_push(memory_arena_handle, sizeof(*action));
	assert(action);

	*action = (struct action_rotate_to){.base = (struct action){.type = AT_ROTATE_TO,
																.duration = duration,
																.loop = loop,
																.start = action_rotate_to_start,
																.update = action_rotate_to_update},
										.to = to};
	return action;
}
