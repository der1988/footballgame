#ifndef STEERING_H
#define STEERING_H

#include "types.h"

v2 steering_seek(v2 position, v2 velocity, f32 max_velocity, v2 target);
v2 steering_arrive(v2 pos, v2 velocity, f32 max_velocity, v2 target, f32 slowing_distance);
v2 steering_pursuit(v2 position, v2 velocity, f32 max_velocity, v2 target, v2 target_velocity);

#endif // !STEERING_H
