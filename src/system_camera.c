#include "system.h"
#include "entity.h"
#include "entity_camera.h"
#include "game.h"

void system_camera(struct game_state *game_state)
{

	FOR_EACH_ENTITY(entity, game_state->entity_repository)
	{
		if (!entity_is_alive_and_has_components(
				entity, 4, C_TRANSFORM, C_PHYSICS, C_TARGET, C_CAMERA)) {
			continue;
		}

		entity_camera_follow_target(entity, game_state);
	}
}
