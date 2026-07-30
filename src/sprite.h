#ifndef SPRITE_H
#define SPRITE_H

#include "components/components.h"
#include "macro.h"
#include "types.h"
#include "math_ext/math_ext.h"

const static v2 ORIENTATION_VECTORS[] = {{.x = 0, .y = -1},
										 {.x = 0, .y = 1},
										 {.x = -1, .y = 0},
										 {.x = 1, .y = 0},
										 {.x = -1, .y = -1},
										 {.x = 1, .y = -1},
										 {.x = -1, .y = 1},
										 {.x = 1, .y = 1}};

INLINE enum sprite_orientation sprite_calculate_orientation(v2 direction)
{
	f32 max_value = 0;
	enum sprite_orientation selected_orientation = SO_DOWN;
	for (u16 i = 0; i < 8; ++i) {
		f32 tmp_value = Vector2ProjectScalar(ORIENTATION_VECTORS[i], direction);
		if (tmp_value > max_value) {
			max_value = tmp_value;
			selected_orientation = i;
		}
	}
	return selected_orientation;
}

INLINE v2 sprite_apply_offset_to_orientation(enum sprite_orientation sprite_orientation, v2 offset)
{
	v2 v = ORIENTATION_VECTORS[sprite_orientation];

	// Calculate the magnitude of the vector
	f32 magnitude = sqrt(v.x * v.x + v.y * v.y);
	if (magnitude == 0)
		return v; // Avoid division by zero

	// Normalize the vector
	v2 v_norm = {v.x / magnitude, v.y / magnitude};

	// Calculate the perpendicular vector
	v2 v_perp = {-v_norm.y, v_norm.x};

	// Transform offset to global coordinates
	v2 o_global = {offset.x * v_perp.x - offset.y * v_norm.x,
				   offset.x * v_perp.y - offset.y * v_norm.y};

	// Apply the offset
	v2 result = {v.x + o_global.x, v.y + o_global.y};
	return result;
}

#endif // !SPRITE_H
