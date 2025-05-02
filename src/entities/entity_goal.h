#include "football.h"

struct goal_config {
	enum pitch_side side;
	v2 center;
};

struct entity *entity_goal_create(struct goal_config config, struct game_state *game_state);
