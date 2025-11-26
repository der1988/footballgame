#include "components.h"
#include "e_ball.h"
#include "e_player.h"
#include "entities.h"
#include "football.h"
#include "game.h"
#include "input.h"
#include "log.h"
#include "macro.h"
#include "ai.h"
#include "types.h"
#include <math_ext.h>
#include <raymath.h>
#include <stdio.h>
#include <float.h>

void systems_player_collisions(struct game_state *game_state)
{
	struct entity *ball_entity = game_state->vars.ball_entity;

	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
	UNUSED(ball_physics_3d);

	// Handle tackles
	FOR_EACH_ENTITY(entity, game_state->entity_repository)
	{
		if (!entity_is_alive_and_has_components(
				entity, 5, C_TRANSFORM, C_PHYSICS, C_SPRITE, C_PLAYER, C_COLLIDER)) {
			continue;
		}

		DECLARE_ENTITY_COMPONENT(entity, player);

		struct state_machine_player *state_machine = &player->state_machine;

		switch (state_machine->current_state) {
		case PS_SLIDING_TACKLE: {
			struct list_entities entities =
				entity_player_find_tackled_opponents(entity, game_state);
			LIST_ITERATE(tackled_player_entity, entities)
			{
				entity_player_state_machine_change_state_to(
					&tackled_player_entity->player.state_machine, PS_FALL, game_state);
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
	f32 min_distance_from_the_ball_possession = FLT_MAX;
	f32 min_distance_from_the_ball[TS_COUNT] = {FLT_MAX, FLT_MAX};

	FOR_EACH_ENTITY(player_entity, game_state->entity_repository)
	{
		if (!entity_is_alive_and_has_components(
				player_entity, 5, C_TRANSFORM, C_PHYSICS, C_SPRITE, C_PLAYER, C_COLLIDER)) {
			continue;
		}

		DECLARE_ENTITY_COMPONENT(player_entity, transform);
		DECLARE_ENTITY_COMPONENT(player_entity, player);
		DECLARE_ENTITY_COMPONENT(player_entity, collider);

		struct state_machine_player *state_machine = &player->state_machine;

		collider->colliders[PCT_PLAYER].props.circle.radius =
			game_state->constants.player_control_radius;

		// Reset variables
		player->is_colliding_with_ball = false;
		player->kicked_ball = false;

		switch (state_machine->current_state) {
		case PS_NORMAL: {
			const struct collision_information collision_with_ball_info =
				entity_player_is_colliding_with_ball(
					player_entity,
					PCT_PLAYER,
					V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
					ball_entity->physics3d.velocity,
					ball_collider->colliders[0].props.circle.radius,
					game_state->constants.player_control_height);

			const f32 distance_from_the_ball =
				Vector2DistanceSqr(transform->position, ball_transform->position);

			struct entity *last_tackler =
				entity_get(game_state->entity_repository, ball->last_tackle_entity_id);

			player->is_colliding_with_ball =
				collision_with_ball_info.is_colliding &&
				HAS_TIME_PASSED(game_state->time, player->last_kick_time, 0.1f) &&
				(!last_tackler ||
				 last_tackler->player.team_side == player_entity->player.team_side ||
				 HAS_TIME_PASSED(game_state->time, ball->last_tackle_time, 0.1f));

			if (player->is_colliding_with_ball) {
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

			break;
		}
		case PS_SLIDING_TACKLE: {
			if (ball->last_tackle_entity_id == player_entity->header.id &&
				!HAS_TIME_PASSED(game_state->time, ball->last_tackle_time, 0.1f)) {
				break;
			}

			struct collider *collider = &player_entity->collider.colliders[PCT_TACKLE_1];

			// Check first tackle collider
			struct collision_information collision_with_ball_info =
				entity_player_is_colliding_with_ball(
					player_entity,
					PCT_TACKLE_1,
					V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
					ball_entity->physics3d.velocity,
					ball_collider->colliders[0].props.circle.radius,
					game_state->constants.player_control_height * 0.25f);

			// If no hit, check second tackle collider
			if (!collision_with_ball_info.is_colliding) {
				collider = &player_entity->collider.colliders[PCT_TACKLE_2];
				collision_with_ball_info = entity_player_is_colliding_with_ball(
					player_entity,
					PCT_TACKLE_2,
					V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
					ball_entity->physics3d.velocity,
					ball_collider->colliders[0].props.circle.radius,
					game_state->constants.player_control_height * 0.5f);
			}

			// If we have a hit set collision information
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

			player->is_colliding_with_ball = collision_with_ball_info.is_colliding;

			break;
		}
		case PS_HEADER: {
			if (ball->last_kicker_entity_id == player_entity->header.id &&
				!HAS_TIME_PASSED(game_state->time, ball->last_kick_time, 0.2f)) {
				break;
			}

			// Check if we are colliding with the ball
			struct collider *collider = &player_entity->collider.colliders[PCT_HEADER];
			struct collision_information collision_with_ball_info =
				entity_player_is_colliding_with_ball(
					player_entity,
					PCT_HEADER,
					V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
					ball_entity->physics3d.velocity,
					ball_collider->colliders[0].props.circle.radius,
					game_state->constants.player_header_max_height);

			// If we have a hit set collision information
			if (collision_with_ball_info.is_colliding) {
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

			player->is_colliding_with_ball = collision_with_ball_info.is_colliding;

			break;
		}
		case PS_GOALIE:
		case PS_THROW_IN:
		case PS_FALL:
		case PS_NONE:
		case PS_COUNT:
			break;
		}
	}

	if (player_on_the_ball_entity) {
		entity_ball_set_last_touch(ball, player_on_the_ball_entity->header.id, game_state);
		entity_player_gained_possession(player_on_the_ball_entity, game_state);
	}
}
