#include "types.h"

// Forward declaration
struct game_state;
struct entity;

enum player_ai_kick_decision {
	PAKD_NO_KICK,
	PAKD_SHOOT,
	PAKD_PASS,
	PAKD_LONG_PASS,
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
									 enum player_ai_kick_decision *kick_decision,
									 f32 *kick_power,
									 b32 *should_tackle,
									 struct game_state *game_state);

enum goalie_ai_state ai_goalie_think(enum player_ai_kick_decision *kick_decision,
									 struct game_state *game_state);
