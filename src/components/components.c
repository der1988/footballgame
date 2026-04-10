#include "components.h"
#include "entities.h"
#include "types.h"
#include "log.h"
#include <assert.h>
#include "game.h"
#include <raymath.h>

const char *camera_type_to_string[CAM_COUNT] = {"main"};

running_action_idx_t component_action_run(entity_id_t id,
										  struct action *action,
										  struct game_state *game_state)
{
	assert(action);

	struct entity *entity = entity_get(game_state->entity_repository, id);

	assert(entity->action.header.enabled);
	struct action_runner *action_runner = NULL;

	running_action_idx_t idx = 0;
	for (idx = 0; idx < MAX_CONCURRENT_ACTIONS; ++idx) {
		action_runner = &entity->action.running_actions[idx];
		if (!action_runner->action) {
			break;
		}
	}
	if (action_runner) {
		action_runner->action = action;
		action_runner->paused = action_runner->completed = action_runner->started = false;
	} else {
		GLOGF_WARN_CTX(LS_ENTITIES,
					   entity->header.name,
					   "no slot available to run action of type %s: too many concurrent actions!",
					   ActionTypeToString[action->type]);
		return -1;
	}
	return idx;
}

void component_action_stop(entity_id_t id,
						   running_action_idx_t running_action_idx,
						   struct game_state *game_state)
{
	struct entity *entity = entity_get(game_state->entity_repository, id);

	entity->action.running_actions[running_action_idx].action = NULL;
}

void component_transform_local_to_world(struct component_transform *transform,
										v2 *position,
										f32 *rotation,
										v2 *scale)
{
	if (!transform->parent) {
		return;
	}
	v2 parent_position = transform->parent->position;
	f32 parent_rotation = transform->parent->rotation;
	v2 parent_scale = transform->parent->scale;

	component_transform_local_to_world(
		transform->parent, &parent_position, &parent_rotation, &parent_scale);

	*position = Vector2Add(Vector2Rotate(*position, parent_rotation * DEG2RAD), parent_position);
	*rotation = *rotation + parent_rotation;
	*scale = Vector2Multiply(*scale, parent_scale);

	return;
}
