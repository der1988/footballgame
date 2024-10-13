#include "gfx.h"

color_t gfx_color_lerp(color_t from, color_t to, f32 t)
{
    color_t c = (color_t){
	.r = from.r + (to.r - from.r) * t,
	.g = from.g + (to.g - from.g) * t,
	.b = from.b + (to.b - from.b) * t,
	.a = from.a + (to.a - from.a) * t,
    };
    return c;
}
