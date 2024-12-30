#include "log.h"
#include "math_ext.h"
#include <assert.h>
#include "entities.h"
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

entity_id_t entities_common_find_pass_target(struct game_state *game_state,
					     struct entity *player_on_the_ball_entity,
					     f32 detection_angle,
					     f32 max_distance)
{
    struct entity_repository *entity_repository = game_state->entity_repository;
    entity_id_t selected_entity_id = EID_NOT_VALID;
    f32 selected_distance = UINT32_MAX;
    f32 selected_angle = UINT32_MAX;
    struct entity *entity;
    for (usize entity_id = 0; entity_id < entity_repository->max_slots; ++entity_id) {
	entity = entities_entity_get(game_state->entity_repository, entity_id);
	if (entity->header.alive && entity->header.type == ET_PLAYER) {

	    struct component_transform *transform = &entity->transform;

	    if (player_on_the_ball_entity->header.id == entity_id ||
		player_on_the_ball_entity->player.team_side != entity->player.team_side) {
		// We don't want to pass to ourselves nor to the other team, right?
		continue;
	    }

	    v2 selected_direction = player_on_the_ball_entity->player_controls.steering;
	    v2 to_player =
		Vector2Subtract(transform->position, player_on_the_ball_entity->transform.position);
	    f32 distance_from_player = Vector2Length(to_player);
	    f32 angle = fabs(Vector2Angle(selected_direction, to_player) * RAD2DEG);
	    if (distance_from_player < max_distance && angle < detection_angle &&
		angle < selected_angle) {
		selected_entity_id = entity_id;
		selected_distance = distance_from_player;
		selected_angle = angle;

		GLOGF_DEBUG(LS_GAME,
			    "found valid pass target: %zu, %.2f, %.2f",
			    entity_id,
			    distance_from_player,
			    angle);
	    }
	}
    }
    if (selected_entity_id != EID_NOT_VALID) {
	GLOGF_DEBUG(LS_GAME,
		    "selected pass target %zu at %.2f with angle %.2f",
		    selected_entity_id,
		    selected_distance,
		    selected_angle);
    }
    return selected_entity_id;
}

entity_id_t entities_common_find_nearest_teammate(struct game_state *game_state,
						  entity_id_t player_to_not_select_entity_id,
						  v2 position,
						  enum team_side team_side)
{
    struct entity *ball_entity =
	entities_entity_get(game_state->entity_repository, game_state->vars.ball_entity_id);
    struct component_ball *ball = &ball_entity->ball;

    struct entity_repository *entity_repository = game_state->entity_repository;
    entity_id_t selected_entity_id = EID_NOT_VALID;
    f32 selected_distance = UINT32_MAX;
    struct entity *entity;
    for (usize entity_id = 0; entity_id < entity_repository->max_slots; ++entity_id) {
	entity = entities_entity_get(game_state->entity_repository, entity_id);
	if (entity->header.alive && entity->header.type == ET_PLAYER) {
	    struct component_transform *transform = &entity->transform;
	    struct component_goalie *goalie = &entity->goalie;

	    if (player_to_not_select_entity_id == entity_id ||
		entity->player.team_side != team_side ||
		(goalie->header.enabled && goalie->state_machine->current_state != GS_HOLD_BALL) ||
		ball->pass_target_entity_id == entity_id) {
		// If player is the one on the ball or a goalie or the destination of the pass let's
		// skip
		continue;
	    }

	    f32 distance = Vector2Distance(position, transform->position);
	    if (distance < selected_distance) {
		selected_entity_id = entity_id;
		selected_distance = distance;
	    }
	}
    }
    return selected_entity_id;
}
