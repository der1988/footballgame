#ifndef STEERING
#define STEERING
#include "steering.h"
#include <raymath.h>

v2 steering_arrive(v2 position, v2 velocity, f32 max_velocity, v2 target, f32 slowing_distance) {
    v2 target_offset = Vector2Subtract(target, position);
    f32 distance = Vector2Length(target_offset);
    f32 ramped_speed = max_velocity * (distance / slowing_distance);
    f32 clipped_speed = fminf(ramped_speed, max_velocity);
    v2 desired_velocity = Vector2Scale(target_offset, clipped_speed / distance);
    return Vector2Subtract(desired_velocity, velocity);
}

#endif
