#include "entity_components.h"
#include "entity.h"
#include "entity_ball.h"
#include "entity_player.h"
#include "football.h"
#include "game.h"
#include "input.h"
#include "macro.h"
#include "math_ext.h"
#include <raymath.h>
#include "ai.h"
#include "system.h"

void system_players_off_the_ball(struct game_state *game_state)
{

	struct entity *ball_entity = game_state->vars.ball.entity;

	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
	UNUSED(ball_physics_3d);
	UNUSED(ball_collider);
	UNUSED(ball_transform);
	UNUSED(ball);

	FOR_EACH_ENTITY(entity, game_state->entity_repository)
	{
		if (!entity_is_alive_and_has_components(
				entity, 5, C_TRANSFORM, C_PHYSICS, C_SPRITE, C_PLAYER, C_COLLIDER)) {
			continue;
		}

		DECLARE_ENTITY_COMPONENT(entity, player);
		DECLARE_ENTITY_COMPONENT(entity, collider);

		struct state_machine_player *state_machine = &player->state_machine;

		struct component_player_controls *player_controls = &entity->player_controls;

		collider->colliders[PCT_PLAYER].props.circle.radius = player->constants.control_radius;

		switch (state_machine->current_state) {
		case PS_NONE:
			break;
		case PS_NORMAL: {
			// Header
			if (player_controls->header_shot || player_controls->header_pass) {
				state_machine_player_change_state_to(
					&player->state_machine, PS_HEADER, game_state);
				player->state_machine.state_data.header.type =
					player_controls->header_shot ? HEADER_TYPE_SHOOT : HEADER_TYPE_PASS;
			}
			// Sliding tackle
			else if (player_controls->sliding_tackle) {
				state_machine_player_change_state_to(
					&player->state_machine, PS_SLIDING_TACKLE, game_state);
			}
			break;
		}
		case PS_SLIDING_TACKLE:
		case PS_FALL:
		case PS_HEADER:
		case PS_GOALIE:
		case PS_THROW_IN:
		case PS_COUNT:
			break;
		}
	}
}
