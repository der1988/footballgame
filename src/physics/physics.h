#ifndef PHYSICS_H
#define PHYSICS_H

#include "types.h"
#include "macro.h"
#include <raymath.h>

// Forward declaration
struct game_state;

void physics_ball_simulation(v3 *out_position,
							 v3 *out_velocity,
							 v3 accel,
							 f32 mass,
							 f32 radius,
							 b32 *out_on_the_ground,
							 b32 *out_bounced,
							 struct game_state *game_state);

INLINE b32 physics_can_reach_position_in_time(v2 current_position,
											  v2 position_to_reach,
											  time_t time,
											  f32 max_speed)
{
	f32 distance_to_cover = Vector2Distance(position_to_reach, current_position);

	return (distance_to_cover < (max_speed * time));
}

#endif // !PHYSICS.H
