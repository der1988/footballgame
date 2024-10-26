#ifndef MATH_EXT_H
#define MATH_EXT_H

#include "types.h"
#include <raymath.h>

#define MAX(a, b)                                                                                  \
    ({                                                                                             \
	__typeof__(a) _a = (a);                                                                    \
	__typeof__(b) _b = (b);                                                                    \
	_a > _b ? _a : _b;                                                                         \
    })

#define MIN(a, b)                                                                                  \
    ({                                                                                             \
	__typeof__(a) _a = (a);                                                                    \
	__typeof__(b) _b = (b);                                                                    \
	_a < _b ? _a : _b;                                                                         \
    })

#define V2_TO_V3(vector2, height)                                                                  \
    (v3) { .x = vector2.x, .y = height, .z = vector2.y }
#define V3_TO_V2(vector3)                                                                          \
    (v2) { .x = vector3.x, .y = vector3.z }

inline __attribute__((always_inline)) Vector2 Vector2Left()
{
    Vector2 v = {.x = -1, .y = 0};
    return v;
}

inline __attribute__((always_inline)) Vector2 Vector2Right()
{
    Vector2 v = {.x = 1, .y = 0};
    return v;
}

inline __attribute__((always_inline)) Vector2 Vector2Up()
{
    Vector2 v = {.x = 0, .y = -1};
    return v;
}

inline __attribute__((always_inline)) Vector2 Vector2Down()
{
    Vector2 v = {.x = 0, .y = 1};
    return v;
}

f32 math_ext_trigo_constrain_angle_360(f32 angle);
f32 math_ext_trigo_constrain_angle_180(f32 angle);
v2 Vector2RotateAroundPivot(v2 vector, v2 pivot, f32 angle);
float Vector2ProjectScalar(Vector2 project_to, Vector2 project_from);
Vector2 Vector2Project(Vector2 project_to, Vector2 project_from);
f32 Vector2ProjectExt(v2 project_to, v2 project_from, v2 *out);
v2 Vector2Perp(v2 vector);
inline __attribute__((always_inline)) f32 Clamp01(f32 value) { return Clamp(value, 0, 1); }

f32 math_ext_split_lerp(f32 value, f32 value_fixed_ratio, f32 amount);

#endif // !MATH_EXT_H
