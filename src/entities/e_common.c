#include "assets.h"
#include "components.h"
#include "game.h"
#include "utils.h"
#include <assert.h>
#include <raylib.h>
#include <stdio.h>

AssetsAnimationId entities_common_animation_id_from_name(char *entity_name, char *animation_name,
                                                         struct GameState *game_state) {
    // Scratch buffer
    char *final_animation =
        memory_arena_push(game_state->memory_by_lifetime[MAL_UPDATE], MaxStringSize);

    snprintf(final_animation, MaxStringSize, "%s_%s", entity_name, animation_name);
    struct AssetsAnimation *animation =
        assets_animation_get_by_name(game_state->animation_cache, final_animation);
    assert(animation);

    return animation->id;
}

enum Direction entities_common_calculate_direction(Vector2 direction) {
    static Vector2 direction_vectors[] = {{.x = 0, .y = -1},
                                          {.x = 0, .y = 1},
                                          {.x = -1, .y = 0},
                                          {.x = 1, .y = 0},
                                          {.x = -1, .y = -1},
                                          {.x = 1, .y = -1},
                                          {.x = -1, .y = 1},
                                          {.x = 1, .y = 1}};
    f32 max_value = 0;
    enum Direction selected_direction = D_DOWN;
    for (u16 i = 0; i < 8; ++i) {
        f32 tmp_value = Vector2ProjectScalar(direction_vectors[i], direction);
        if (tmp_value > max_value) {
            max_value = tmp_value;
            selected_direction = i;
        }
    }
    return selected_direction;
}
