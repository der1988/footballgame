#include "football.h"
#include "math_ext.h"
#include <assert.h>
#include "entities.h"
#include <raymath.h>
#include <stdio.h>
#include "game.h"

assets_animation_id_t entities_common_animation_id_from_name(const char *entity_name,
							     const char *animation_name,
							     struct game_state *game_state)
{
    // Scratch buffer
    char *final_animation =
	memory_arena_push(game_state->memory_by_lifetime[MAL_UPDATE], MaxStringSize);

    snprintf(final_animation, MaxStringSize, "%s_%s", entity_name, animation_name);
    struct assets_animation *animation =
	assets_animation_get_by_name(game_state->assets.animation_cache, final_animation);
    assert(animation);

    return animation->id;
}

enum direction entities_common_calculate_direction(Vector2 direction)
{
    static Vector2 direction_vectors[] = {{.x = 0, .y = -1},
					  {.x = 0, .y = 1},
					  {.x = -1, .y = 0},
					  {.x = 1, .y = 0},
					  {.x = -1, .y = -1},
					  {.x = 1, .y = -1},
					  {.x = -1, .y = 1},
					  {.x = 1, .y = 1}};
    f32 max_value = 0;
    enum direction selected_direction = D_DOWN;
    for (u16 i = 0; i < 8; ++i) {
	f32 tmp_value = Vector2ProjectScalar(direction_vectors[i], direction);
	if (tmp_value > max_value) {
	    max_value = tmp_value;
	    selected_direction = i;
	}
    }
    return selected_direction;
}

INLINE b32 entities_common_can_reach_position_in_time(v2 current_position,
						      v2 position_to_reach,
						      time_t time,
						      f32 max_speed)
{
    f32 distance_to_cover = Vector2Distance(position_to_reach, current_position);

    return (distance_to_cover < (max_speed * time));
}
