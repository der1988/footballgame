#ifndef AI_H
#define AI_H

#include "types.h"

// Forward declaration
struct game_state;
struct entity;

enum player_ai_action_decision {
	PAAD_NO_ACTION,
	PAAD_SHOOT,
	PAAD_PASS,
	PAAD_LONG_PASS,
	PAAD_TACKLE,
	PAAD_HEADER_SHOOT,
	PAAD_HEADER_PASS,
};

enum player_ai_state {
	PAS_KEEP_POSITION,
	PAS_SEEK_POSITION,
	PAS_ARRIVE_TO_POSITION,
	PAS_SET_STEERING,
};

enum goalie_ai_state {
	GAS_PROTECT_GOAL,
	GAS_HOLD_BALL,
	GAS_DIVE
};

enum player_ai_state ai_player_think(struct entity *entity,
									 v2 *target_position,
									 enum player_ai_action_decision *action_decision,
									 f32 *power,
									 struct game_state *game_state);

enum goalie_ai_state ai_goalie_think(enum player_ai_action_decision *action_decision,
									 struct game_state *game_state);

#endif // !AI_H
