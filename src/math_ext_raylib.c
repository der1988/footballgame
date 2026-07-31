#include "types.h"
#include <raymath.h>

v2 Vector2RotateAroundPivot(v2 vector, v2 pivot, f32 angle)
{
	v2 new_vector = Vector2Subtract(pivot, vector);
	new_vector = Vector2Rotate(new_vector, DEG2RAD * (angle + 180));
	return Vector2Add(new_vector, pivot);
}

// Projects a vector onto another and then returns the scalar value
f32 Vector2ProjectScalar(v2 project_to, v2 project_from)
{
	return (project_to.x * project_from.x + project_to.y * project_from.y) /
		   Vector2Length(project_to);
}

// Projects a vector onto another
v2 Vector2Project(v2 project_to, v2 project_from)
{
	return Vector2Scale(Vector2Normalize(project_to),
						Vector2ProjectScalar(project_to, project_from));
}

f32 Vector2ProjectExt(v2 project_to, v2 project_from, v2 *out)
{
	f32 scalar = Vector2ProjectScalar(project_to, project_from);
	*out = Vector2Scale(Vector2Normalize(project_to), scalar);
	return scalar;
}

v2 Vector2Perp(v2 vector) { return (v2){.x = vector.y, .y = -vector.x}; }
