#include "components.h"
#include "e_ball.h"
#include "e_player.h"
#include "entities.h"
#include "football.h"
#include "game.h"
#include "input.h"
#include "log.h"
#include "macro.h"
#include <math_ext.h>
#include <raymath.h>
#include "ai.h"
#include "types.h"

void systems_player_collisions(struct game_state *game_state)
{
	struct entity *ball_entity =
		entities_entity_get(game_state->entity_repository, game_state->vars.ball_entity_id);
	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
	UNUSED(ball_physics_3d);

	game_state->vars.player_on_the_ball_entity_id = EID_NOT_VALID;
	struct entity *player_on_the_ball_entity = NULL;

	f32 min_distance_from_the_ball_possession = UINT32_MAX;
	f32 min_distance_from_the_ball[TS_COUNT] = {UINT32_MAX, UINT32_MAX};

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
			const b32 is_colliding_with_ball = entities_player_is_colliding_with_ball(
				entity,
				PCT_PLAYER,
				V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
				ball_collider->colliders[0].props.circle.radius,
				game_state->constants.player_control_height);

			const f32 distance_from_the_ball =
				Vector2DistanceSqr(transform->position, ball_transform->position);

			if (is_colliding_with_ball &&
				HAS_TIME_PASSED(game_state->time, player->last_kick_time, 0.1f)) {
				if (distance_from_the_ball < min_distance_from_the_ball_possession) {
					player_on_the_ball_entity = entity;
					min_distance_from_the_ball_possession = distance_from_the_ball;

					ball->pass_target_entity_id = EID_NOT_VALID;
				}
			} else {
				player->control_ball = true;
				player->time_spent_in_precision_control = 0;

				if (distance_from_the_ball < min_distance_from_the_ball[entity->player.team_side]) {
					game_state->vars.player_nearest_to_the_ball[entity->player.team_side] =
						entity_id;
					min_distance_from_the_ball[entity->player.team_side] = distance_from_the_ball;
				}
			}
			player->is_colliding_with_ball = is_colliding_with_ball;

			break;
		}
		case PS_SLIDING_TACKLE: {
			const b32 is_colliding_with_ball = entities_player_is_colliding_with_ball(
				entity,
				PCT_TACKLE,
				V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
				ball_collider->colliders[0].props.circle.radius,
				game_state->constants.player_control_height);

			if (is_colliding_with_ball) {
				player->is_colliding_with_ball = true;
			}
			struct entities entities = entities_player_find_tackled_opponents(entity, game_state);
			for (usize i = 0; i < entities.length; ++i) {
				entities_player_state_machine_change_state_to(
					entities.array[i]->player.state_machine, PS_FALL, game_state);
				GLOGF_INFO_CTX(LS_GAME,
							   entities.array[i]->header.name,
							   "player tackled by %s",
							   entity->header.name);
			}

			break;
		}
		case PS_GOALIE:
		case PS_THROW_IN:
		case PS_HEADER:
		case PS_FALL:
		case PS_NONE:
		case PS_COUNT:
			break;
		}
	}

	if (player_on_the_ball_entity) {
		game_state->vars.player_on_the_ball_entity_id = player_on_the_ball_entity->header.id;
		entities_player_gained_possession(player_on_the_ball_entity, game_state);
	}
}
