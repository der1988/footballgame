#ifndef COLLISION_H
#define COLLISION_H

#include "macro.h"
#include "types.h"

const static v2 ORIENTATION_TO_TACKLE_COLLIDER_OFFSET[] = {{.x = 0, .y = -14},	 // SO_UP
														   {.x = 0, .y = 8},	 // SO_DOWN
														   {.x = -10, .y = 0},	 // SO_LEFT
														   {.x = 10, .y = 0},	 // SO_RIGHT
														   {.x = -10, .y = -10}, // SO_UP_LEFT
														   {.x = 10, .y = -10},	 // SO_UP_RIGHT
														   {.x = -10, .y = 0},	 // SO_DOWN_LEFT
														   {.x = 10, .y = 0}};	 // SO_DOWN_RIGHT
																				 //
INLINE rect_t calculate_collision_rectangle(v2 pos, v2 size, v2 offset)
{
	return (rect_t){.x = pos.x - size.x * 0.5f + offset.x * 0.5f,
					.y = pos.y - size.y * 0.5f + offset.y * 0.5f,
					.width = size.x,
					.height = size.y};
}

#endif // !COLLISION_H
