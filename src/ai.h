#ifndef AI_H
#define AI_H

#include "football.h"
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

// Where the keeper is going while he is the goalie
enum goalie_ai_state {
	GAS_NONE,		// nothing to steer: holding still, diving, jumping, following through
	GAS_STAND,		// on the arc between the ball and his goal centre
	GAS_COVER_LINE, // where the ball will cross his line, at half speed
	GAS_CLAIM_BALL, // running to where he can get to the ball
	GAS_CARRY,		// walking out with the ball in his hands
};

enum goalie_ai_action {
	GAA_NO_ACTION,
	GAA_THROW,	  // roll it out to a team mate
	GAA_KICK,	  // clear it upfield
	GAA_PUT_DOWN, // set it down and play on with his feet
	GAA_JUMP,	  // go up for a high ball he can reach where he stands
	GAA_DIVE,	  // dive for a ball going past him
};

struct goalie_ai_decision {
	enum goalie_ai_state state;
	v2 steering;
	enum goalie_ai_action action;
	// Only the member named by action is valid
	union {
		struct {
			f32 power; // what a human's charge would have been
			entity_id_t target_entity_id;
		} throw_ball;
		struct {
			f32 power; // what a human's charge would have been
		} kick;
		struct goalie_dive dive;
	} action_data;
};

struct player_ai_decision {
	enum player_ai_state state;
	v2 target_position; // where he goes, for PAS_SEEK_POSITION and PAS_ARRIVE_TO_POSITION
	v2 steering;		// the stick he holds, for PAS_SET_STEERING
	enum player_ai_action_decision action;
	f32 power;
	entity_id_t target_entity_id; // who he passes to for PAAD_PASS, who he goes in on for PAAD_TACKLE
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
