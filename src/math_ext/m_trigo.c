#include "types.h"
#include <math.h>
#include <raylib.h>
#include <raymath.h>

f32 math_ext_trigo_constrain_angle_360(f32 angle)
{
    angle = fmodf(angle, 360);
    if (angle < 0)
	angle += 360;
    return angle;
}

f32 math_ext_trigo_constrain_angle_180(f32 angle)
{
    angle = fmodf(angle + 180, 360);
    if (angle < 0)
	angle += 360;
    return angle - 180;
}
