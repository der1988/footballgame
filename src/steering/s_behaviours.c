#include "steering.h"
#include <raymath.h>

v2 steering_seek(v2 position, v2 velocity, f32 max_velocity, v2 target)
{
    v2 normalized_target_offset = Vector2Normalize(Vector2Subtract(target, position));
    v2 desired_velocity = Vector2Scale(normalized_target_offset, max_velocity);
    v2 steering = Vector2Subtract(desired_velocity, velocity);
    return Vector2Scale(steering, 1.0f / max_velocity);
}

v2 steering_arrive(v2 position, v2 velocity, f32 max_velocity, v2 target, f32 slowing_distance)
{
    v2 target_offset = Vector2Subtract(target, position);
    f32 distance = Vector2Length(target_offset);
    f32 clipped_speed = max_velocity * Clamp(distance / slowing_distance, 0, 1);
    v2 desired_velocity = Vector2Scale(target_offset, clipped_speed / distance);
    v2 steering = Vector2Subtract(desired_velocity, velocity);
    return Vector2Scale(steering, 1.0f / max_velocity);
}

v2 steering_pursuit(v2 position, v2 velocity, f32 max_velocity, v2 target, v2 target_velocity)
{
    v2 target_offset = Vector2Subtract(target, position);

    // the lookahead time is proportional to the distance between the evader
    // and the pursuer; and is inversely proportional to the sum of the
    // agent's velocities
    f32 look_ahead_time =
	Vector2Length(target_offset) / (max_velocity + Vector2Length(target_velocity));

    // now seek to the predicted future position of the evader
    return steering_seek(position,
			 velocity,
			 max_velocity,
			 Vector2Add(target, Vector2Scale(target_velocity, look_ahead_time)));
}
