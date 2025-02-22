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
#include <stdio.h>
#include "ai.h"
#include "types.h"

void systems_player_collisions(struct game_state *game_state)
{
	struct entity *ball_entity =
		entities_entity_get(game_state->entity_repository, game_state->vars.ball_entity_id);
	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
	UNUSED(ball_physics_3d);

	// Handle tackles
	FOR_EACH_ENTITY(entity, game_state->entity_repository)
	{
		if (!entities_is_entity_alive_and_has_components(
				entity, 5, C_TRANSFORM, C_PHYSICS, C_SPRITE, C_PLAYER, C_COLLIDER)) {
			continue;
		}

		DECLARE_ENTITY_COMPONENT(entity, player);

		struct state_machine_player *state_machine = player->state_machine;

		switch (state_machine->current_state) {
		case PS_SLIDING_TACKLE: {
			struct list_entities entities =
				entities_player_find_tackled_opponents(entity, game_state);
			LIST_ITERATE(tackled_player_entity, entities)
			{
				entities_player_state_machine_change_state_to(
					tackled_player_entity->player.state_machine, PS_FALL, game_state);
				GLOGF_INFO_CTX(LS_GAME,
							   entities.array[i]->header.name,
							   "player tackled by %s",
							   entity->header.name);
			}

			break;
		}
		case PS_NORMAL:
		case PS_GOALIE:
		case PS_THROW_IN:
		case PS_HEADER:
		case PS_FALL:
		case PS_NONE:
		case PS_COUNT:
			break;
		}
	}

	// Handle collisions
	struct entity *player_on_the_ball_entity = NULL;
	f32 min_distance_from_the_ball_possession = UINT32_MAX;
	f32 min_distance_from_the_ball[TS_COUNT] = {UINT32_MAX, UINT32_MAX};

	FOR_EACH_ENTITY(player_entity, game_state->entity_repository)
	{
		if (!entities_is_entity_alive_and_has_components(
				player_entity, 5, C_TRANSFORM, C_PHYSICS, C_SPRITE, C_PLAYER, C_COLLIDER)) {
			continue;
		}

		DECLARE_ENTITY_COMPONENT(player_entity, transform);
		DECLARE_ENTITY_COMPONENT(player_entity, player);
		DECLARE_ENTITY_COMPONENT(player_entity, collider);

		struct state_machine_player *state_machine = player->state_machine;

		collider->colliders[PCT_PLAYER].props.circle.radius =
			game_state->constants.player_control_radius;

		// Reset variables
		player->is_colliding_with_ball = false;
		player->kicked_ball = false;

		switch (state_machine->current_state) {
		case PS_NORMAL: {
			const struct collision_information collision_with_ball_info =
				entities_player_is_colliding_with_ball(
					player_entity,
					PCT_PLAYER,
					V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
					ball_entity->physics3d.velocity,
					ball_collider->colliders[0].props.circle.radius,
					game_state->constants.player_control_height);

			const f32 distance_from_the_ball =
				Vector2DistanceSqr(transform->position, ball_transform->position);

			if (collision_with_ball_info.is_colliding &&
				HAS_TIME_PASSED(game_state->time, player->last_kick_time, 0.1f)) {
				if (distance_from_the_ball < min_distance_from_the_ball_possession) {
					player_on_the_ball_entity = player_entity;
					min_distance_from_the_ball_possession = distance_from_the_ball;
				}
			} else {
				player->control_ball = true;
				player->time_spent_in_precision_control = 0;

				if (distance_from_the_ball <
					min_distance_from_the_ball[player_entity->player.team_side]) {
					game_state->vars.teams[player_entity->player.team_side]
						.play_context.player_nearest_to_the_ball = player_entity_id;
					min_distance_from_the_ball[player_entity->player.team_side] =
						distance_from_the_ball;
				}
			}
			player->is_colliding_with_ball = collision_with_ball_info.is_colliding;

			break;
		}
		case PS_SLIDING_TACKLE: {
			struct collider *collider = &player_entity->collider.colliders[PCT_TACKLE_1];
			struct collision_information collision_with_ball_info =
				entities_player_is_colliding_with_ball(
					player_entity,
					PCT_TACKLE_1,
					V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
					ball_entity->physics3d.velocity,
					ball_collider->colliders[0].props.circle.radius,
					game_state->constants.player_control_height * 0.5f);

			if (!collision_with_ball_info.is_colliding) {
				collider = &player_entity->collider.colliders[PCT_TACKLE_2];
				collision_with_ball_info = entities_player_is_colliding_with_ball(
					player_entity,
					PCT_TACKLE_2,
					V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
					ball_entity->physics3d.velocity,
					ball_collider->colliders[0].props.circle.radius,
					game_state->constants.player_control_height);
			}

			if (collision_with_ball_info.is_colliding) {
				player->is_colliding_with_ball = true;
				player->collision_direction = collision_with_ball_info.direction;
				player->collision_force = collision_with_ball_info.force;
				const f32 distance_from_the_ball = Vector2DistanceSqr(
					Vector2Add(player_entity->transform.position, collider->props.circle.offset),
					ball_transform->position);
				if (distance_from_the_ball < min_distance_from_the_ball_possession) {
					player_on_the_ball_entity = player_entity;
					min_distance_from_the_ball_possession = distance_from_the_ball;
				}
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
		// TODO: (marco) should we move the following line into entities_ball_set_last_touch function?
		ball->pass_target_entity_id = EID_NOT_VALID;
		entities_ball_set_last_touch(ball, player_on_the_ball_entity->header.id, game_state);
		entities_player_gained_possession(player_on_the_ball_entity, game_state);
	}
}
