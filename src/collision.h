#include "types.h"

const static v2 ORIENTATION_TO_TACKLE_COLLIDER_OFFSET[] = {{.x = 0, .y = -14},	 // SO_UP
							   {.x = 0, .y = 8},	 // SO_DOWN
							   {.x = -10, .y = 0},	 // SO_LEFT
							   {.x = 10, .y = 0},	 // SO_RIGHT
							   {.x = -10, .y = -10}, // SO_UP_LEFT
							   {.x = 10, .y = -10},	 // SO_UP_RIGHT
							   {.x = -10, .y = 0},	 // SO_DOWN_LEFT
							   {.x = 10, .y = 0}};	 // SO_DOWN_RIGHT
