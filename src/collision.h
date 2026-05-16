#ifndef COLLISION_H
#define COLLISION_H

#include "macro.h"
#include "raymath.h"
#include "types.h"

struct tackle_colliders_config {
	struct {
		v2 offset;
		f32 radius;
	} collider_1;
	struct {
		v2 offset;
		f32 radius;
	} collider_2;
};

const static struct tackle_colliders_config ORIENTATION_TO_TACKLE_COLLIDERS_CONFIG[] = {
	{{{.x = 0, .y = -8}, 4}, {{.x = 0, .y = 2}, 5}},	// SO_UP
	{{{.x = -1, .y = 5}, 4}, {{.x = 0, .y = -4}, 5}},	// SO_DOWN
	{{{.x = -8, .y = 0}, 4}, {{.x = 1, .y = -1}, 4}},	// SO_LEFT
	{{{.x = 8, .y = 0}, 4}, {{.x = -1, .y = -1}, 4}},	// SO_RIGHT
	{{{.x = -10, .y = -7}, 3}, {{.x = 0, .y = -4}, 6}}, // SO_UP_LEFT
	{{{.x = 10, .y = -7}, 3}, {{.x = 0, .y = -4}, 6}},	// SO_UP_RIGHT
	{{{.x = -7, .y = 2}, 4}, {{.x = 2, .y = -3}, 5}},	// SO_DOWN_LEFT
	{{{.x = 7, .y = 2}, 4}, {{.x = -2, .y = -3}, 5}},	// SO_DOWN_RIGHT
};

INLINE rect_t calculate_collision_rectangle(v2 pos, v2 size, v2 offset)
{
	return (rect_t){.x = pos.x - size.x * 0.5f + offset.x * 0.5f,
					.y = pos.y - size.y * 0.5f + offset.y * 0.5f,
					.width = size.x,
					.height = size.y};
}

// This function determines whether a moving circle with a velocity is going to intercept another still circle
INLINE b32 is_moving_circle_going_to_intersect_still_circle(v2 moving_circle_pos,
															v2 still_circle_pos,
															f32 moving_circle_radius,
															f32 still_circle_radius,
															v2 moving_circle_velocity)
{
	const f32 radius_sum = moving_circle_radius + still_circle_radius;
	const v2 w = Vector2Subtract(moving_circle_pos, still_circle_pos);
	const f32 A = Vector2LengthSqr(moving_circle_velocity);
	const f32 B = 2 * Vector2DotProduct(w, moving_circle_velocity);
	const f32 C = Vector2LengthSqr(w) - radius_sum * radius_sum;
	const f32 discriminant = B * B - 4 * A * C;
	if (discriminant >= 0 && B <= 0) {
		return true;
	}

	return false;
}

INLINE b32 is_line_intersecting_line(v2 line_1_point_1,
									 v2 line_1_point_2,
									 v2 line_2_point_1,
									 v2 line_2_point_2,
									 v2 *intersection_point)
{
	float D = (line_1_point_1.x - line_1_point_2.x) * (line_2_point_1.y - line_2_point_2.y) -
			  (line_1_point_1.y - line_1_point_2.y) * (line_2_point_1.x - line_2_point_2.x);
	if (D != 0) {
		f32 t = ((line_1_point_1.x - line_2_point_1.x) * (line_2_point_1.y - line_2_point_2.y) -
				 (line_1_point_1.y - line_2_point_1.y) * (line_2_point_1.x - line_2_point_2.x)) /
				D;
		if (t >= 0 && t <= 1) {
			*intersection_point =
				(v2){.x = line_1_point_1.x + t * (line_1_point_2.x - line_1_point_1.x),
					 .y = line_1_point_1.y + t * (line_1_point_2.y - line_1_point_1.y)};

			return true;
		}
	}
	return false;
}

#endif // !COLLISION_H
