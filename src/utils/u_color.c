#include "../types.h"
#include <raylib.h>

Color utils_color_lerp(Color from, Color to, f32 t) {
    Color c = (Color){
        .r = from.r + (to.r - from.r) * t,
        .g = from.g + (to.g - from.g) * t,
        .b = from.b + (to.b - from.b) * t,
        .a = from.a + (to.a - from.a) * t,
    };
    return c;
}
