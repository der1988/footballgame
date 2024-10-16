#include "assets.h"
#include "components.h"
#include "game.h"
#include "math_ext.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include "constants.h"
#include "entities.h"
#include <stdio.h>

assets_animation_id_t entities_common_animation_id_from_name(const char *entity_name,
							     const char *animation_name,
							     struct game_state *game_state)
{
    // Scratch buffer
    char *final_animation =
	memory_arena_push(game_state->memory_by_lifetime[MAL_UPDATE], MaxStringSize);

    snprintf(final_animation, MaxStringSize, "%s_%s", entity_name, animation_name);
    struct assets_animation *animation =
	assets_animation_get_by_name(game_state->animation_cache, final_animation);
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

b32 entities_common_can_reach_position_in_time(v2 current_position,
					       v2 position_to_reach,
					       time_t time,
					       f32 max_speed)
{
    f32 distance_to_cover = Vector2Distance(position_to_reach, current_position);

    return (distance_to_cover < (max_speed * time));
}

b32 entities_common_can_reach_future_ball_position(struct entity *seeker_entity,
						   f32 seeker_max_speed,
						   v3 *future_ball_positions,
						   b32(predicate)(v3 pos_to_check,
								  struct entity *seeker_entity,
								  struct game_state *game_state),
						   v3 *future_pos,
						   time_t *future_time,
						   struct game_state *game_state)
{
    const int steps_per_lookahead =
	(game_state->vars.slow_motion ? SLOW_MOTION_LOOKAHEAD_MULTIPLIER : 1) *
	game_state->constants.ball_lookahead_dts;
    const int lookaheads = game_state->constants.ball_lookaheads;
    for (u16 i = 0; i < lookaheads; ++i) {
	v3 pos_to_check = future_ball_positions[i];
	if (!predicate || predicate(pos_to_check, seeker_entity, game_state)) {
	    time_t lookahead_time = (i + 1) * game_state->dt * steps_per_lookahead;
	    if (entities_common_can_reach_position_in_time(seeker_entity->transform.position,
							   V3_TO_V2(pos_to_check),
							   lookahead_time,
							   seeker_max_speed)) {
		*future_pos = pos_to_check;
		*future_time = lookahead_time;
		return true;
	    }
	}
    }
    return false;
}
