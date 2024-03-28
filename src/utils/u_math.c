#include "../types.h"
#include <math.h>
#include <raylib.h>
#include <raymath.h>

f32 utils_math_constrain_angle_360(f32 angle) {
    angle = fmodf(angle, 360);
    if (angle < 0)
        angle += 360;
    return angle;
}

f32 utils_math_constrain_angle_180(f32 angle) {
    angle = fmodf(angle + 180, 360);
    if (angle < 0)
        angle += 360;
    return angle - 180;
}

v2 Vector2RotateAroundPivot(v2 vector, v2 pivot, f32 angle) {
    v2 new_vector = Vector2Subtract(pivot, vector);
    new_vector = Vector2Rotate(new_vector, DEG2RAD * (angle + 180));
    return Vector2Add(new_vector, pivot);
}

f32 Vector2ProjectScalar(v2 project_to, v2 project_from) {
    return (project_to.x * project_from.x + project_to.y * project_from.y) /
           Vector2Length(project_to);
}

v2 Vector2Project(v2 project_to, v2 project_from) {
    return Vector2Scale(Vector2Normalize(project_to),
                        Vector2ProjectScalar(project_to, project_from));
}
