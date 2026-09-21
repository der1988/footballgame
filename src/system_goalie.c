#include "system.h"
#include "entity_components.h"
#include "entity_player.h"
#include "entity.h"
#include "game.h"
#include "types.h"

void system_goalie(struct game_state *game_state)
{
	FOR_EACH_ENTITY(entity, game_state->entity_repository)
	{
		if (!entity_is_alive_and_has_components(
				entity, 6, C_TRANSFORM, C_PHYSICS, C_SPRITE, C_PLAYER, C_GOALIE, C_COLLIDER)) {
			continue;
		}

		entity_player_goalie_update_role(entity, game_state);
	}
}
