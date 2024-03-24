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

float Vector2ProjectScalar(Vector2 project_to, Vector2 project_from) {
    return (project_to.x * project_from.x + project_to.y * project_from.y) /
           Vector2Length(project_to);
}

Vector2 Vector2Project(Vector2 project_to, Vector2 project_from) {
    return Vector2Scale(Vector2Normalize(project_to),
                        Vector2ProjectScalar(project_to, project_from));
}
