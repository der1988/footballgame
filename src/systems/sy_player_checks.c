#include "components.h"
#include "entities.h"
#include "football.h"
#include "game.h"
#include "input.h"
#include "macro.h"
#include <math_ext.h>
#include <raymath.h>
#include "ai.h"
#include "types.h"

void systems_player_checks(struct game_state *game_state)
{
    struct entity *ball_entity =
	entities_entity_get(game_state->entity_repository, game_state->vars.ball_entity_id);
    DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
    UNUSED(ball_physics_3d);

    game_state->vars.player_on_the_ball_entity_id = EID_NOT_VALID;

    f32 min_distance_from_the_ball = UINT32_MAX;

    FOR_EACH_ENTITY(entity, game_state->entity_repository)
    {
	if (!entities_is_entity_alive_and_has_components(
		entity, 5, C_TRANSFORM, C_PHYSICS, C_SPRITE, C_PLAYER, C_COLLIDER)) {
	    continue;
	}

	DECLARE_ENTITY_COMPONENT(entity, transform);
	DECLARE_ENTITY_COMPONENT(entity, player);
	DECLARE_ENTITY_COMPONENT(entity, collider);

	struct state_machine_player *state_machine = player->state_machine;

	collider->colliders[PCT_PLAYER].props.circle.radius =
	    game_state->constants.player_control_radius;

	player->is_colliding_with_ball = false;
	player->kicked_ball = false;

	switch (state_machine->current_state) {
	case PS_NORMAL: {
	    b32 is_colliding_with_ball =
		CheckCollisionCircles(transform->position,
				      collider->colliders[0].props.circle.radius,
				      ball_transform->position,
				      ball_collider->colliders[0].props.circle.radius) &&
		// Player height
		ball_transform->virtual_height < game_state->constants.player_control_height;

	    if (is_colliding_with_ball &&
		HAS_TIME_PASSED(game_state->time, player->last_kick_time, 0.1f)) {
		f32 distance_from_the_ball =
		    Vector2DistanceSqr(transform->position, ball_transform->position);
		if (distance_from_the_ball < min_distance_from_the_ball) {
		    game_state->vars.player_on_the_ball_entity_id = entity_id;
		    min_distance_from_the_ball = distance_from_the_ball;

		    ball->pass_target_entity_id = EID_NOT_VALID;
		}
	    } else {
		player->control_ball = true;
		player->time_spent_in_precision_control = 0;
	    }
	    player->is_colliding_with_ball = is_colliding_with_ball;

	    break;
	}
	case PS_NONE:
	case PS_GOALIE:
	case PS_THROW_IN:
	case PS_HEADER:
	case PS_SLIDING_TACKLE:
	case PS_FALL:
	case PS_COUNT:
	    break;
	}
    }
}
