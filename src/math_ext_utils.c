#include "math_ext.h"
#include "types.h"

f32 math_ext_split_lerp(f32 value, f32 value_fixed_ratio, f32 amount)
{
	return value * (value_fixed_ratio + (1 - value_fixed_ratio) * amount);
}
