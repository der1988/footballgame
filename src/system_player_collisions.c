#include "system.h"
#include "entity_components.h"
#include "entity_ball.h"
#include "entity_player.h"
#include "entity.h"
#include "football.h"
#include "gameplay.h"
#include "game.h"
#include "physics.h"
#include "input.h"
#include "log.h"
#include "macro.h"
#include "ai.h"
#include "types.h"
#include "math_ext.h"
#include <raymath.h>
#include <stdio.h>
#include <float.h>
#include "constants.h"

void system_player_collisions(struct game_state *game_state)
{
	struct entity *ball_entity = game_state->vars.ball.entity;

	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
	UNUSED(ball_physics_3d);

	struct entity_repository *repo = game_state->entity_repository;

	FOR_EACH_ENTITY(entity, repo)
	{
		if (entity->header.alive && entity->header.type == ET_PLAYER) {
			entity_player_update_colliders(entity);
		}
	}

	// Handle tackles
	FOR_EACH_ENTITY(tackling_player_entity, repo)
	{
		if (!tackling_player_entity->header.alive ||
			tackling_player_entity->header.type != ET_PLAYER)
			continue;

		struct state_machine_player *tackler_state_machine =
			&tackling_player_entity->player.state_machine;

		if (tackler_state_machine->current_state == PS_TACKLE) {
			// A standing tackle only ever hits the man it went in on, once
			struct state_machine_player_tackle_data *tackle_data =
				&tackler_state_machine->state_data.tackle;
			if (tackle_data->touched_player) {
				continue;
			}

			struct entity *victim_entity = entity_get(repo, tackle_data->victim_entity_id);
			// He may have changed state since the tackle started: only a man on his feet or in a
			// tackled run can be hit
			const enum state_machine_player_state victim_state =
				victim_entity->player.state_machine.current_state;
			if (victim_entity->header.alive &&
				(victim_state == PS_NORMAL || victim_state == PS_TACKLED_RUN) &&
				collision_check_colliders_overlap(
					tackling_player_entity->transform.position,
					&tackling_player_entity->collider.colliders[PCT_TACKLE],
					victim_entity->transform.position,
					&victim_entity->collider.colliders[PCT_TACKLE_SENSOR])) {
				if (!tackle_data->touched_ball && !tackling_player_entity->player.is_colliding_with_ball) {
					gameplay_award_foul(tackling_player_entity, victim_entity, game_state);
					if (game_state->gameplay.restart.prepared) return;
				}
				entity_player_tackled(victim_entity,
									  tackling_player_entity,
									  TACKLE_KNOCKDOWN_PROBABILITY,
									  game_state);
				tackle_data->touched_player = true;
				tackle_data->victim_touch_time = game_state->time;
			}
		} else if (tackler_state_machine->current_state == PS_SLIDING_TACKLE) {
			struct list_entities entities = entity_player_find_tackled_opponents(
				tackling_player_entity, PCT_SLIDING_TACKLE_1, game_state);
			LIST_ITERATE(tackled_player_entity, entities)
			{
				if (tackler_state_machine->current_state == PS_SLIDING_TACKLE &&
					!tackler_state_machine->state_data.sliding_tackle.touched_ball &&
					!tackling_player_entity->player.is_colliding_with_ball) {
					gameplay_award_foul(tackling_player_entity, tackled_player_entity, game_state);
					if (game_state->gameplay.restart.prepared) return;
				}
				entity_player_tackled(tackled_player_entity,
									  tackling_player_entity,
									  SLIDING_TACKLE_KNOCKDOWN_PROBABILITY,
									  game_state);
			}
		} else if (tackler_state_machine->current_state == PS_GOALIE &&
				   tackling_player_entity->goalie.state_machine.current_state == GS_SMOTHER) {
			struct list_entities entities = entity_player_find_tackled_opponents(
				tackling_player_entity, PCT_SMOTHER, game_state);
			LIST_ITERATE(tackled_player_entity, entities)
			{
				if (tackler_state_machine->current_state == PS_SLIDING_TACKLE &&
					!tackler_state_machine->state_data.sliding_tackle.touched_ball &&
					!tackling_player_entity->player.is_colliding_with_ball) {
					gameplay_award_foul(tackling_player_entity, tackled_player_entity, game_state);
					if (game_state->gameplay.restart.prepared) return;
				}
				entity_player_tackled(tackled_player_entity,
									  tackling_player_entity,
									  GOALIE_SMOTHER_KNOCKDOWN_PROBABILITY,
									  game_state);
			}
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

		struct state_machine_player *state_machine = &player->state_machine;

		// Reset variables
		player->is_colliding_with_ball = false;
		player->kicked_ball = false;

		switch (state_machine->current_state) {
		case PS_NORMAL: {
			const struct collision_information collision_with_ball_info =
				entity_player_is_colliding_with_ball(
					player_entity, PCT_PLAYER, player->constants.control_height, game_state);

			const f32 distance_from_the_ball =
				Vector2DistanceSqr(transform->position, ball_transform->position);

			// The man a tackle took the ball off is kept off it for a while, even when the tackle
			// left him on his feet, so he can't take it straight back.
			player->is_colliding_with_ball =
				collision_with_ball_info.is_colliding &&
				// A ball in the keeper's hands is nobody else's to touch. It rides below control
				// height, so without this anyone brushing past him takes the last touch off him.
				ball->state_machine.current_state != BS_GK_HANDS &&
				HAS_TIME_PASSED(game_state->time, player->last_kick_time, 0.1f) &&
				HAS_TIME_PASSED(game_state->time,
								player->last_tackle_dispossession_time,
								TACKLE_DISPOSSESSION_BALL_LOCK_TIME);

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
			if (ball->state_machine.current_state == BS_GK_HANDS) {
				// Not his to win while the keeper is holding it
				break;
			}

			// A challenge plays the ball once. The state outlasts any timer short enough to let
			// contact through again, so what has already been played is what stops it.
			if (state_machine->state_data.sliding_tackle.touched_ball) {
				break;
			}

			struct collider *collider = &player_entity->collider.colliders[PCT_SLIDING_TACKLE_1];

			// Check first tackle collider
			struct collision_information collision_with_ball_info =
				entity_player_is_colliding_with_ball(player_entity,
													 PCT_SLIDING_TACKLE_1,
													 player->constants.control_height * 0.25f,
													 game_state);

			// If no hit, check second tackle collider
			if (!collision_with_ball_info.is_colliding) {
				collider = &player_entity->collider.colliders[PCT_SLIDING_TACKLE_2];
				collision_with_ball_info =
					entity_player_is_colliding_with_ball(player_entity,
														 PCT_SLIDING_TACKLE_2,
														 player->constants.control_height * 0.5f,
														 game_state);
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
			if (ball->state_machine.current_state == BS_GK_HANDS) {
				// Not his to head while the keeper is holding it
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
					player_entity, PCT_HEADER, HEADER_MAX_HEIGHT, game_state);

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
		case PS_TACKLE: {
			if (ball->state_machine.current_state == BS_GK_HANDS) {
				// Not his to win while the keeper is holding it
				break;
			}

			if (state_machine->state_data.tackle.touched_ball) {
				break;
			}

			const struct collision_information collision_with_ball_info =
				entity_player_is_colliding_with_ball(
					player_entity, PCT_TACKLE, player->constants.control_height, game_state);

			player->is_colliding_with_ball = collision_with_ball_info.is_colliding;
			player->collision_direction = collision_with_ball_info.direction;
			player->collision_force = collision_with_ball_info.force;

			break;
		}
		case PS_TACKLED_RUN: {
			if (ball->state_machine.current_state == BS_GK_HANDS) {
				// Not his to touch while the keeper is holding it
				break;
			}

			// He can only touch the ball while he is still the last player who touched it. Once
			// someone else touches it (the tackler too), he can't get it back until he recovers.
			if (ball->last_touch_entity_id != player_entity_id) {
				break;
			}

			const struct collision_information collision_with_ball_info =
				entity_player_is_colliding_with_ball(
					player_entity, PCT_PLAYER, player->constants.control_height, game_state);

			player->is_colliding_with_ball = collision_with_ball_info.is_colliding;

			if (player->is_colliding_with_ball) {
				const f32 distance_from_the_ball =
					Vector2DistanceSqr(transform->position, ball_transform->position);
				if (distance_from_the_ball < min_distance_from_the_ball_possession) {
					player_on_the_ball_entity = player_entity;
					min_distance_from_the_ball_possession = distance_from_the_ball;
				}
			}

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

	// Goalies last, so a ball a goalie gets to is his whoever else touched it on this tick
	FOR_EACH_ENTITY(goalie_entity, repo)
	{
		if (!goalie_entity->header.alive || goalie_entity->header.type != ET_PLAYER ||
			goalie_entity->player.state_machine.current_state != PS_GOALIE) {
			continue;
		}

		goalie_entity->player.is_colliding_with_ball =
			entity_player_goalie_is_colliding_with_ball(goalie_entity, game_state);
		if (goalie_entity->player.is_colliding_with_ball) {
			player_on_the_ball_entity = goalie_entity;
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
			player_entity_a->player.state_machine.current_state == PS_SLIDING_TACKLE ||
			(player_entity_a->player.state_machine.current_state == PS_GOALIE &&
			 player_entity_a->goalie.state_machine.current_state == GS_SMOTHER))
			continue;

		f32 radius_a = player_entity_a->collider.colliders[PCT_BODY].props.circle.radius;

		for (usize j = i + 1; j < repo->max_slots; ++j) {
			struct entity *player_entity_b = entity_get(repo, j);
			if (!player_entity_b->header.alive || player_entity_b->header.type != ET_PLAYER)
				continue;
			if (player_entity_b->player.state_machine.current_state == PS_FALL ||
				player_entity_b->player.state_machine.current_state == PS_SLIDING_TACKLE ||
				(player_entity_b->player.state_machine.current_state == PS_GOALIE &&
				 player_entity_b->goalie.state_machine.current_state == GS_SMOTHER))
				continue;

			f32 radius_b = player_entity_b->collider.colliders[PCT_BODY].props.circle.radius;

			f32 impulse_magnitude = physics_resolve_body_collision(&player_entity_a->transform,
																   &player_entity_a->physics,
																   radius_a,
																   &player_entity_b->transform,
																   &player_entity_b->physics,
																   radius_b,
																   bounce_factor);
			if (impulse_magnitude > 0) {
				player_entity_a->player.last_body_collision_time = game_state->time;
				player_entity_a->player.last_body_collision_impulse = impulse_magnitude;
				player_entity_b->player.last_body_collision_time = game_state->time;
				player_entity_b->player.last_body_collision_impulse = impulse_magnitude;
			}
		}
	}
}
