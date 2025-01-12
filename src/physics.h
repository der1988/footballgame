#ifndef PHYSICS_H
#define PHYSICS_H

#include "macro.h"
#include "types.h"
#include <raymath.h>

INLINE b32 physics_can_reach_position_in_time(v2 current_position,
											  v2 position_to_reach,
											  time_t time,
											  f32 max_speed)
{
	f32 distance_to_cover = Vector2Distance(position_to_reach, current_position);

	return (distance_to_cover < (max_speed * time));
}

#endif // !PHYSICS_H
