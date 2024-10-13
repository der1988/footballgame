#include "ai.h"
#include "macro.h"

enum goalie_ai_state ai_goalie_think(enum player_ai_kick_decision *kick_decision,
				     struct game_state *game_state)
{
    UNUSED(kick_decision);
    UNUSED(game_state);

    return GAS_PROTECT_GOAL;
}
