#ifndef ENTITIES_CONFIG_H
#define ENTITIES_CONFIG_H

#include "types.h"

typedef struct {
    v2 position;
    b32 is_goalie;
} PlayerConfig;

typedef struct {
    v2 position;
} BallConfig;

typedef enum { GP_TOP, GP_BOTTOM } GoalPosition;

typedef struct {
    GoalPosition position;
} GoalConfig;

#endif // !ENTITIES_CONFIG_H
