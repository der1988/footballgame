#include "../types.h"

usize utils_align(usize to_align, u16 alignment) {
    if (alignment > 0) {
        if (to_align < alignment) {
            to_align = alignment;
        } else {
            // If to_align cannot be divided by alignment fix it
            to_align =
                to_align & alignment ? to_align + alignment - to_align % alignment : to_align;
        }
    }
    return to_align;
}
