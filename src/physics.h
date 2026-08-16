#ifndef PHYSICS_H
#define PHYSICS_H

#include "types.h"
#include "macro.h"
#include <raymath.h>

// Forward declaration
struct game_state;
struct component_physics;
struct component_transform;

// Semi-implicit Euler on a 2D body. The max velocity clamp is deliberately not part of
// applying a force: the camera integrates unclamped, players do not.
void physics_apply_force(struct component_physics *physics, v2 force, delta_time_t dt);
void physics_clamp_velocity_to_max(struct component_physics *physics);
void physics_apply_velocity(struct component_transform *transform,
							struct component_physics *physics,
							delta_time_t dt);

// Force opposing motion, of the given magnitude. Below a speed of magnitude * dt / mass a
// tick of this reverses the motion instead of damping it, so callers stop applying it and
// put the body to rest at that point rather than let it coast.
v2 physics_drag_force(v2 velocity, f32 magnitude);

// Whether a body is slow enough to be considered stopped.
INLINE b32 physics_is_at_rest(v2 velocity, f32 stop_speed)
{
	return Vector2LengthSqr(velocity) <= stop_speed * stop_speed;
}

// Parks the body at rest if it is slow enough, and reports whether it did.
b32 physics_set_at_rest_if_needed(struct component_physics *physics, f32 stop_speed);

// Separates two overlapping circular bodies along their centre line, weighted by inverse
// mass, and applies a bounce impulse if they are closing. Returns the impulse magnitude,
// or 0 when they do not overlap or are already moving apart.
f32 physics_resolve_body_collision(struct component_transform *transform_a,
								   struct component_physics *physics_a,
								   f32 radius_a,
								   struct component_transform *transform_b,
								   struct component_physics *physics_b,
								   f32 radius_b,
								   f32 bounce_factor);

void physics_ball_simulation(v3 *out_position,
							 v3 *out_velocity,
							 v3 accel,
							 f32 mass,
							 f32 radius,
							 v3 after_touch_accel,
							 b32 *out_on_the_ground,
							 b32 *out_bounced,
							 struct game_state *game_state);

INLINE b32 physics_can_reach_position_in_time(v2 current_position,
											  v2 position_to_reach,
											  fg_time_t time,
											  f32 max_speed)
{
	f32 distance_to_cover = Vector2Distance(position_to_reach, current_position);

	return (distance_to_cover < (max_speed * time));
}

#endif // !PHYSICS.H
