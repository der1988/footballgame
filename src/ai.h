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
	entity_id_t target_entity_id; // who he throws to, for GAA_THROW
};

struct player_ai_decision {
	enum player_ai_state state;
	v2 target_position; // where he goes, for PAS_SEEK_POSITION and PAS_ARRIVE_TO_POSITION
	v2 steering;		// the stick he holds, for PAS_SET_STEERING
	enum player_ai_action_decision action;
	f32 power;
	entity_id_t pass_target_entity_id; // who he passes to, for PAAD_PASS
};

struct player_ai_decision ai_player_think(struct entity *entity, struct game_state *game_state);

struct goalie_ai_decision ai_goalie_think(struct entity *goalie_entity,
										  struct game_state *game_state);

b32 ai_pass_find_target(struct entity *passer_entity,
						v2 scan_direction,
						f32 detection_angle,
						f32 target_distance,
						f32 max_distance,
						b32 require_clear_trajectory,
						struct entity **out_pass_target,
						struct game_state *game_state);

#endif // !AI_H
