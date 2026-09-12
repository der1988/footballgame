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
	PAAD_SLIDING_TACKLE,
	PAAD_HEADER_SHOOT,
	PAAD_HEADER_PASS,
};

enum player_ai_state {
	PAS_KEEP_POSITION,
	PAS_SEEK_POSITION,
	PAS_ARRIVE_TO_POSITION,
	PAS_SET_STEERING,
};

// What the keeper does with the ball once it is in his hands. His goalkeeping is not decided here:
// protecting the goal, coming for loose balls, diving and jumping all live in system_goalie.
enum goalie_ai_action {
	GAA_HOLD,	  // keep hold of it a moment longer
	GAA_CARRY,	  // walk out with it before releasing
	GAA_THROW,	  // roll it out to a team mate
	GAA_KICK,	  // clear it upfield
	GAA_PUT_DOWN, // set it down and play on with his feet
};

struct goalie_ai_decision {
	enum goalie_ai_action action;
	v2 steering; // where he walks, for GAA_CARRY
	f32 power;	 // what a human's charge would have been, for GAA_THROW and GAA_KICK
};

enum player_ai_state ai_player_think(struct entity *entity,
									 v2 *target_position,
									 enum player_ai_action_decision *action_decision,
									 f32 *power,
									 struct game_state *game_state);

struct goalie_ai_decision ai_goalie_think(struct entity *goalie_entity,
										  struct game_state *game_state);

#endif // !AI_H
