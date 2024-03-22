#include "../components/components.h"
#include <raymath.h>

void components_transform_calculate_absolute(ComponentTransform *transform, Vector2 *position,
                                             f32 *rotation, Vector2 *scale) {
    if (!transform->parent) {
        return;
    }
    Vector2 parent_position = transform->parent->position;
    f32 parent_rotation = transform->parent->rotation;
    Vector2 parent_scale = transform->parent->scale;

    components_transform_calculate_absolute(transform->parent, &parent_position, &parent_rotation,
                                            &parent_scale);

    *position = Vector2Add(Vector2Rotate(*position, parent_rotation * DEG2RAD), parent_position);
    *rotation = *rotation + parent_rotation;
    *scale = Vector2Multiply(*scale, parent_scale);

    return;
}
