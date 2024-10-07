#include "game.h"
#include "entities.h"

enum PlayerAIKickDecision
{
    PAKD_NO_KICK,
    PAKD_SHOOT,
    PAKD_PASS,
    PAKD_LONG_PASS,
};

enum PlayerAIState
{
    PAS_KEEP_POSITION,
    PAS_SEEK_POSITION,
    PAS_ARRIVE_TO_POSITION,
};

enum GoalieAIState
{
    GAS_PROTECT_GOAL,
    GAS_HOLD_BALL,
    GAS_DIVE
};

enum PlayerAIState ai_player_think(struct Entity *entity, v2 *target_position,
				   enum PlayerAIKickDecision *kick_decision,
				   struct GameState *game_state);

enum GoalieAIState ai_goalie_think(enum PlayerAIKickDecision *kick_decision,
				   struct GameState *game_state);
