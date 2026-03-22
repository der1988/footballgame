#include "ai.h"
#include "macro.h"

enum goalie_ai_state ai_goalie_think(enum player_ai_action_decision *action_decision,
									 struct game_state *game_state)
{
	UNUSED(action_decision);
	UNUSED(game_state);

	return GAS_PROTECT_GOAL;
}
