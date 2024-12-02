#include "entities.h"
#include "football.h"
#include "game.h"
#include "types.h"
#include <raymath.h>
#include "ai.h"

static entity_id_t find_nearest_teammate(struct game_state *game_state,
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

void systems_player_selection(struct game_state *game_state, struct input *input)
{
    UNUSED(input);

    struct entity *ball_entity =
	entities_entity_get(game_state->entity_repository, game_state->vars.ball_entity_id);
    declare_ball_entity_components(ball_entity);
    UNUSED(ball_physics_3d);
    UNUSED(ball_collider);

    struct player_on_the_ball *player_on_the_ball = &game_state->vars.player_on_the_ball;

    // Select human controlled player
    entity_id_t nearest_teammate_entity_id = player_on_the_ball->id;
    entity_id_t player_to_avoid_entity_id = EID_NOT_VALID;
    if (!HAS_TIME_PASSED(game_state->time, ball->last_kick_time, 0.5f)) {
	player_to_avoid_entity_id = ball->last_kicker_entity_id;
    }
    v2 epicenter = ball_transform->position;
    if (Vector2Distance(epicenter, ball->future_bounce.position) > 100) {
	// If the ball is in flight and it's gonna bounce far from current ball position,
	// let's use bounce position instead to select which player is human controlled
	epicenter = ball->future_bounce.position;
    }
    if (player_on_the_ball->kicked_ball || player_on_the_ball->id == EID_NOT_VALID) {
	nearest_teammate_entity_id = find_nearest_teammate(
	    game_state, player_to_avoid_entity_id, epicenter, game_state->vars.p1.team_side);
    }
    game_state->vars.p1.player_entity_id = nearest_teammate_entity_id;
}
