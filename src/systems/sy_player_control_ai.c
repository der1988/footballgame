#include "components.h"
#include "entities.h"
#include "football.h"
#include "game.h"
#include "input.h"
#include "macro.h"
#include "steering.h"
#include <math_ext.h>
#include <raymath.h>
#include <string.h>
#include "ai.h"
#include "types.h"

void systems_player_control_ai(struct game_state *game_state)
{
    UNUSED(memset);

    struct entity *ball_entity =
	entities_entity_get(game_state->entity_repository, game_state->vars.ball_entity_id);
    DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
    UNUSED(ball_physics_3d);
    UNUSED(ball_collider);
    UNUSED(ball_transform);
    UNUSED(ball);

    FOR_EACH_ENTITY(entity, game_state->entity_repository)
    {
	if (game_state->human_players[0].controlled_player_entity_id == entity_id) {
	    continue;
	}

	if (!entities_is_entity_alive_and_has_components(
		entity, 5, C_TRANSFORM, C_PHYSICS, C_SPRITE, C_PLAYER, C_COLLIDER)) {
	    continue;
	}

	DECLARE_ENTITY_COMPONENT(entity, transform);
	DECLARE_ENTITY_COMPONENT(entity, physics);
	DECLARE_ENTITY_COMPONENT(entity, player);
	DECLARE_ENTITY_COMPONENT(entity, collider);

	struct state_machine_player *state_machine = player->state_machine;

	struct component_player_controls *player_controls = &entity->player_controls;
	RESET_COMPONENT(player_controls);

	collider->colliders[PCT_PLAYER].props.circle.radius =
	    game_state->constants.player_control_radius;

	switch (state_machine->current_state) {
	case PS_NONE:
	    break;
	case PS_NORMAL: {

	    // Player is AI controlled
	    if (game_state->constants.ai_enabled) {
		v2 target_pos;
		enum player_ai_kick_decision kick_decision;
		enum player_ai_state ai_state =
		    ai_player_think(entity, &target_pos, &kick_decision, game_state);

		switch (ai_state) {
		case PAS_KEEP_POSITION:
		    player_controls->steering = Vector2Zero();
		    break;
		case PAS_SEEK_POSITION: {
		    player_controls->steering = steering_seek(
			transform->position, physics->velocity, physics->max_velocity, target_pos);
		    break;
		}
		case PAS_ARRIVE_TO_POSITION: {
		    player_controls->steering = steering_arrive(transform->position,
								physics->velocity,
								physics->max_velocity,
								target_pos,
								20);
		    break;
		}
		}
	    }

	    break;
	}
	case PS_GOALIE:
	case PS_THROW_IN:
	case PS_HEADER:
	case PS_SLIDING_TACKLE:
	case PS_FALL:
	case PS_COUNT:
	    break;
	}
    }
}
