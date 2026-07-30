#include "components/components.h"
#include "entities/e_ball.h"
#include "entities/e_player.h"
#include "entities/entities.h"
#include "football/football.h"
#include "game/game.h"
#include "input/input.h"
#include "log/log.h"
#include "macro.h"
#include "ai/ai.h"
#include "types.h"
#include "math_ext/math_ext.h"
#include <raymath.h>
#include <stdio.h>
#include <float.h>

void systems_player_collisions(struct game_state *game_state)
{
	struct entity *ball_entity = game_state->vars.ball.entity;

	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
	UNUSED(ball_physics_3d);

	struct entity_repository *repo = game_state->entity_repository;

	// Handle tackles
	FOR_EACH_ENTITY(tackling_player_entity, repo)
	{
		if (!tackling_player_entity->header.alive ||
			tackling_player_entity->header.type != ET_PLAYER)
			continue;

		DECLARE_ENTITY_COMPONENT(tackling_player_entity, player);

		struct state_machine_player *state_machine = &player->state_machine;

		if (state_machine->current_state != PS_SLIDING_TACKLE) {
			continue;
		}

		struct list_entities entities =
			entity_player_find_tackled_opponents(tackling_player_entity, game_state);
		LIST_ITERATE(tackled_player_entity, entities)
		{
			player_state_machine_change_state_to(
				&tackled_player_entity->player.state_machine, PS_FALL, game_state);
			GLOGF_INFO_CTX(LS_GAME,
						   entities.array[tackled_player_entity_i]->header.name,
						   "player tackled by %s",
						   tackling_player_entity->header.name);
		}
	}

	// Handle ball collisions
	struct entity *player_on_the_ball_entity = NULL;
	f32 min_distance_from_the_ball_possession = FLT_MAX;
	f32 min_distance_from_the_ball[TS_COUNT] = {FLT_MAX, FLT_MAX};

	FOR_EACH_ENTITY(player_entity, repo)
	{
		if (!player_entity->header.alive || player_entity->header.type != ET_PLAYER)
			continue;

		DECLARE_ENTITY_COMPONENT(player_entity, transform);
		DECLARE_ENTITY_COMPONENT(player_entity, player);
		DECLARE_ENTITY_COMPONENT(player_entity, collider);

		struct state_machine_player *state_machine = &player->state_machine;

		collider->colliders[PCT_PLAYER].props.circle.radius = player->constants.control_radius;

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
					player->constants.control_height);

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
					player->constants.control_height * 0.25f);

			// If no hit, check second tackle collider
			if (!collision_with_ball_info.is_colliding) {
				collider = &player_entity->collider.colliders[PCT_TACKLE_2];
				collision_with_ball_info = entity_player_is_colliding_with_ball(
					player_entity,
					PCT_TACKLE_2,
					V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
					ball_entity->physics3d.velocity,
					ball_collider->colliders[0].props.circle.radius,
					player->constants.control_height * 0.5f);
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

			struct state_machine_player_header_data *header_data =
				&player_entity->player.state_machine.state_data.header;

			if (header_data->phase == HP_GROUND || header_data->hit_ball) {
				// Cannot hit if we are preparing the jump or we already hit the ball
				break;
			}

			const f32 ball_radius = ball_collider->colliders[0].props.circle.radius;

			struct header_phase_window hpw = header_phase_windows[header_data->phase - 1];

			const f32 ball_height = ball_transform->virtual_height;
			if (ball_height < (hpw.low - ball_radius) || ball_height > (hpw.high + ball_radius)) {
				// Ball is outside header phase height window
				GLOGF_TRACE_CTX(
					LS_GAME,
					player_entity->header.name,
					"missed header, ball outside header phase window: %.2f not in [%.2f - %.2f]\n",
					ball_height,
					hpw.low - ball_radius,
					hpw.high + ball_radius);
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
					HEADER_MAX_HEIGHT);

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
		entity_player_gained_possession(player_on_the_ball_entity, game_state);
	}

	// Handle player-to-player body collisions
	const f32 bounce_factor = 0.45f;

	for (usize i = 0; i < repo->max_slots; ++i) {
		struct entity *player_entity_a = entity_get(repo, i);
		if (!player_entity_a->header.alive || player_entity_a->header.type != ET_PLAYER)
			continue;
		if (player_entity_a->player.state_machine.current_state == PS_FALL ||
			player_entity_a->player.state_machine.current_state == PS_SLIDING_TACKLE)
			continue;

		f32 mass_a = player_entity_a->physics.mass;
		f32 radius_a = player_entity_a->collider.colliders[PCT_BODY].props.circle.radius;

		for (usize j = i + 1; j < repo->max_slots; ++j) {
			struct entity *player_entity_b = entity_get(repo, j);
			if (!player_entity_b->header.alive || player_entity_b->header.type != ET_PLAYER)
				continue;
			if (player_entity_b->player.state_machine.current_state == PS_FALL ||
				player_entity_b->player.state_machine.current_state == PS_SLIDING_TACKLE)
				continue;

			f32 mass_b = player_entity_b->physics.mass;
			f32 radius_b = player_entity_b->collider.colliders[PCT_BODY].props.circle.radius;

			v2 delta = Vector2Subtract(player_entity_b->transform.position,
									   player_entity_a->transform.position);
			f32 dist = Vector2Length(delta);
			f32 min_dist = radius_a + radius_b;

			if (dist >= min_dist || dist < 0.001f)
				continue;

			v2 normal = Vector2Scale(delta, 1.0f / dist);
			f32 overlap = min_dist - dist;

			// Separation weighted by inverse mass
			f32 total_mass = mass_a + mass_b;
			f32 ratio_a = mass_b / total_mass;
			f32 ratio_b = mass_a / total_mass;

			player_entity_a->transform.position = Vector2Subtract(
				player_entity_a->transform.position, Vector2Scale(normal, overlap * ratio_a));
			player_entity_b->transform.position = Vector2Add(
				player_entity_b->transform.position, Vector2Scale(normal, overlap * ratio_b));

			// Velocity impulse when players approach each other
			f32 relative_vel = Vector2DotProduct(Vector2Subtract(player_entity_a->physics.velocity,
																 player_entity_b->physics.velocity),
												 normal);
			if (relative_vel > 0) {
				f32 impulse = relative_vel * (1.0f + bounce_factor) / total_mass;
				player_entity_a->physics.velocity = Vector2Subtract(
					player_entity_a->physics.velocity, Vector2Scale(normal, impulse * mass_b));
				player_entity_b->physics.velocity = Vector2Add(
					player_entity_b->physics.velocity, Vector2Scale(normal, impulse * mass_a));

				f32 impulse_magnitude = impulse * total_mass;
				player_entity_a->player.last_body_collision_time = game_state->time;
				player_entity_a->player.last_body_collision_impulse = impulse_magnitude;
				player_entity_b->player.last_body_collision_time = game_state->time;
				player_entity_b->player.last_body_collision_impulse = impulse_magnitude;
			}
		}
	}
}
