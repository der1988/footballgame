#ifndef ENTITY_GOAL_H
#define ENTITY_GOAL_H

#include "football.h"

struct goal_config {
	enum pitch_side side;
	v2 center;
};

enum goal_woodwork {
	GW_NONE, // passed clean through the mouth
	GW_LEFT_POST,
	GW_RIGHT_POST,
	GW_CROSSBAR,
};

struct goal_collision {
	b32 hit;					 // the trajectory met the goal plane inside the frame
	v3 point;					 // where it met it — meaningful whenever hit
	b32 scored;
	enum goal_woodwork woodwork; // GW_NONE = clean through the mouth
	struct {					 // unused unless woodwork != GW_NONE
		v3 velocity;			 // how the ball comes off, post-restitution
		v3 position;			 // where to place it
	} deflection;
};

struct entity *entity_goal_create(struct goal_config config, struct game_state *game_state);
rect_t entity_goal_rectangle(struct entity *goal_entity);
struct goal_collision entity_goal_resolve_ball_impact(struct entity *goal_entity,
													  struct entity *ball_entity,
													  struct game_state *game_state);

#endif // !ENTITY_GOAL_H
