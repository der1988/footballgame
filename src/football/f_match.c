#include "components.h"
#include "entities.h"
#include "football.h"
#include "game.h"
#include "math_ext.h"
#include "log.h"
#include <raylib.h>
#include <raymath.h>

void football_match_change_state(struct game_state *game_state, enum match_state new_state)
{
    game_state->vars.match.state = new_state;
    GLOGF_INFO(LS_GAME, "match state changed to: %s", match_state_to_string[new_state]);
    switch (new_state)
    {
    case MS_KICKOFF:
    case MS_THROW_IN:
    case MS_CORNER_KICK:
    {
	// TODO: quick hack to put the ball back in play
	struct entity *entity = entities_entity_get(game_state->entity_repository,
						    game_state->vars.p1.player_entity_id);
	struct component_transform *transform = &entity->transform;
	entities_ball_set_position_and_velocity(game_state->vars.ball_entity_id,
						V2_TO_V3(transform->position, 30), Vector3Zero(),
						game_state);
	game_state->vars.match.state = MS_PLAY;
	break;
    }
    default:
	break;
    }
}
