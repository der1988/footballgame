#include "ai.h"
#include "components.h"
#include "e_ball.h"
#include "e_player.h"
#include "entities.h"
#include "football.h"
#include "math_ext.h"
#include "steering.h"
#include "types.h"
#include "game.h"
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <assert.h>

#define STAY_THERE_RADIUS 30.0f

enum whos_on_the_ball {
	WOTB_PLAYER,
	WOTB_TEAMMATE,
	WOTB_OPPONENT,
	WOTB_NO_ONE
};

enum whos_on_the_ball identify_whos_in_possession(struct entity *player_entity,
												  struct entity *player_in_possession_entity);

v2 calculate_position_to_seek_to_reach_ball(struct entity *player_entity,
											struct entity *ball_entity,
											struct game_state *game_state);

v2 get_target_position_based_on_tactics(struct entity *entity,
										v2 ball_position,
										struct game_state *game_state);

enum player_ai_state ai_player_think(struct entity *player_entity,
									 v2 *target_position,
									 enum player_ai_kick_decision *kick_decision,
									 f32 *kick_power,
									 b32 *should_tackle,
									 struct game_state *game_state)
{
	enum player_ai_state ai_state;
	*should_tackle = false;

	struct entity *ball_entity = game_state->vars.ball_entity;
	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
	UNUSED(ball_collider);
	UNUSED(ball_physics_3d);

	DECLARE_ENTITY_COMPONENT(player_entity, transform);

	b32 ball_in_keeper_hands = ball->state_machine.current_state == BS_GK_HANDS;

	entity_id_t player_on_the_ball_entity_id =
		entity_ball_player_on_the_ball(ball_entity, game_state);
	entity_id_t player_in_possession_entity_id =
		entity_ball_player_in_possession(ball_entity, game_state);
	struct entity *player_in_possession_entity =
		entity_get(game_state->entity_repository, player_in_possession_entity_id);

	if (game_state->vars.match.state != MS_PLAY || ball_in_keeper_hands) {
		return PAS_KEEP_POSITION;
	}

	enum whos_on_the_ball whos_on_the_ball =
		player_in_possession_entity_id == EID_NOT_VALID ?
			WOTB_NO_ONE :
			identify_whos_in_possession(player_entity, player_in_possession_entity);

	switch (whos_on_the_ball) {
	case WOTB_PLAYER: {
		// We possess the ball
		b32 should_be_human_controlled = false;
		for (usize i = 0; i < MAX_HUMAN_PLAYERS; ++i) {
			if (player_entity->player.team_side == game_state->human_players[i].team_side) {
				// Player should really be controlled by human
				should_be_human_controlled = true;
				break;
			}
		}
		if (should_be_human_controlled) {
			ai_state = PAS_KEEP_POSITION;
			break;
		}
		if (player_on_the_ball_entity_id == EID_NOT_VALID) {
			// We are not on the ball, so we just chase it to gain control again
			ai_state = PAS_SEEK_POSITION;
			*target_position = ball_transform->position;
			break;
		}

		v2 goal_center = game_state->vars.pitch
							 .goal[game_state->vars.teams[1 - player_entity->player.team_side]
									   .play_context.pitch_side]
							 .center;
		f32 distance_from_goal = Vector2Distance(goal_center, player_entity->transform.position);
		if (distance_from_goal < 250) {
			// f32 accuracy =
			// 	40 + GetRandomValue(
			// 			 0, 20); // TODO: this should really depend on ai level and player stats

			// v2 shot_direction_to_center =
			// 	Vector2Normalize(Vector2Subtract(goal_center, ball_transform->position));
			// v2 shot_direction_to_right_post = Vector2Normalize(Vector2Subtract(
			// 	Vector2Add(goal_center, (v2){.x = accuracy}), ball_transform->position));
			// f32 angle_center_to_post =
			// 	Vector2Angle(shot_direction_to_center, shot_direction_to_right_post);

			// v2 shot_direction = Vector2Rotate(
			// 	shot_direction_to_center, angle_center_to_post * GetRandomValue(-100, 100) / 100.f);

			v2 to_target_goal = Vector2Subtract(goal_center, player_entity->transform.position);
			f32 angle =
				fabs(Vector2Angle(player_entity->physics.velocity, to_target_goal) * RAD2DEG);
			if (angle < 30) {
				*kick_decision = PAKD_SHOOT;
				*kick_power = 0.5f + 0.5f * RANDOM_01(100);
				v2 shoot_target = Vector2Zero();
				shoot_target.x += GetRandomValue(-60, 60) / 60.f;
				*target_position = shoot_target;
				// *target_position = shot_direction;
			} else {
				*target_position = Vector2Normalize(
					Vector2Subtract(goal_center, player_entity->transform.position));
			}
		} else {
			struct nearest_player *opponent_entity =
				entity_player_find_nearest_facing_opponent(player_entity, game_state);
			struct entity *teammate_entity =
				entity_player_find_pass_target(player_entity, 35, 300, game_state);
			if (opponent_entity && teammate_entity) {
				// Facing an opponent and we have someone to pass to, let's pass
				*kick_decision = PAKD_PASS;
				*kick_power = 0.1f;
				*target_position = Vector2Normalize(Vector2Subtract(
					teammate_entity->transform.position, player_entity->transform.position));
			} else {
				*kick_decision = PAKD_NO_KICK;
				*target_position = Vector2Normalize(
					Vector2Subtract(goal_center, player_entity->transform.position));
			}
		}
		ai_state = PAS_SET_STEERING;
		break;
	}
	case WOTB_TEAMMATE: {
		// Let's move based on the tactics of the team
		// TODO: we should probably make runs or movements to facilitate passing here
		if (ball->pass_target_entity_id == player_entity->header.id) {
			// This player is the target of an automatic pass
			// Let's move towards the ball
			*target_position =
				calculate_position_to_seek_to_reach_ball(player_entity, ball_entity, game_state);
			ai_state = PAS_SEEK_POSITION;
		} else {
			ai_state = PAS_KEEP_POSITION;
		}
		break;
	}
	case WOTB_OPPONENT: {
		if (game_state->vars.teams[player_entity->player.team_side]
					.play_context.player_nearest_to_the_ball == player_entity->header.id ||
			game_state->vars.teams[player_entity->player.team_side]
					.play_context.player_in_charge_of_pressing == player_entity->header.id) {
			// We are the nearest to the player with the ball, let's seek the ball and potentially tackle it
			ai_state = PAS_SET_STEERING;
			*target_position = steering_pursuit(player_entity->transform.position,
												player_entity->physics.velocity,
												player_entity->physics.max_velocity,
												ball_transform->position,
												V3_TO_V2(ball_physics_3d->velocity));

			// Consider tackling if this is a fully AI-controlled team
			b32 team_has_human = false;
			for (usize i = 0; i < MAX_HUMAN_PLAYERS; ++i) {
				if (player_entity->player.team_side == game_state->human_players[i].team_side) {
					team_has_human = true;
					break;
				}
			}
			if (!team_has_human) {
				f32 dist_to_ball =
					Vector2Distance(player_entity->transform.position, ball_transform->position);
				if (dist_to_ball < 20.0f && player_in_possession_entity) {
					// Check that player is moving toward the ball
					v2 to_ball = Vector2Subtract(ball_transform->position,
												 player_entity->transform.position);
					f32 dot = Vector2DotProduct(Vector2Normalize(player_entity->physics.velocity),
												Vector2Normalize(to_ball));
					if (dot < 0.866f)
						break; // Not heading toward the ball, skip tackle

					v2 carrier_vel = player_in_possession_entity->physics.velocity;
					v2 carrier_to_tackler =
						Vector2Subtract(player_entity->transform.position,
										player_in_possession_entity->transform.position);
					f32 angle = fabs(Vector2Angle(carrier_vel, carrier_to_tackler) * RAD2DEG);
					// Minimum 0.1% probability per tick if tackling from behind, maximum 1.5% if tackling frontally
					f32 tackle_probability = 0.001f + angle / 180.0f * 0.014f;
					if (RANDOM_01(100000) < tackle_probability) {
						*should_tackle = true;
					}
				}
			}
		} else {
			// We are not the nearest to the player with the ball, so let's mind our business
			// and move based on the tactics of the team
			ai_state = PAS_KEEP_POSITION;
		}
		break;
	}
	case WOTB_NO_ONE: {
		struct future_position *ball_future_reachable_position =
			&ball->future_positions[FUTURE_POSITION_REACHABLE_TO_PLAYER];

		if (ball->pass_target_entity_id == player_entity->header.id) {
			// This player is the target of an automatic pass
			// Let's move towards the ball
			*target_position =
				calculate_position_to_seek_to_reach_ball(player_entity, ball_entity, game_state);
			ai_state = PAS_SEEK_POSITION;
		} else if (ball_future_reachable_position->available &&
				   Vector2Distance(transform->position,
								   V3_TO_V2(ball_future_reachable_position->position)) < 50) {
			*target_position = V3_TO_V2(ball_future_reachable_position->position);
			ai_state = PAS_ARRIVE_TO_POSITION;
		} else if (game_state->vars.teams[player_entity->player.team_side]
					   .play_context.player_nearest_to_the_ball == player_entity->header.id) {
			struct player_ball_reachability_prediction prediction =
				entity_player_can_reach_future_ball_position(
					player_entity,
					game_state->constants.player_run_max_velocity,
					ball->lookahead_positions,
					NULL,
					game_state);
			v3 ball_future_pos;
			if (prediction.can_reach_it)
				ball_future_pos = prediction.future_pos;
			else
				ball_future_pos =
					V2_TO_V3(ball_transform->position, ball_transform->virtual_height);
			*target_position = V3_TO_V2(ball_future_pos);
			ai_state = PAS_SEEK_POSITION;
		} else {
			// Let's move based on the tactics of the team
			ai_state = PAS_KEEP_POSITION;
		}

		*kick_decision = PAKD_NO_KICK;
		break;
	}
	}

	return ai_state;
}

enum whos_on_the_ball identify_whos_in_possession(struct entity *player_entity,
												  struct entity *player_in_possession_entity)
{
	if (player_entity->header.id == player_in_possession_entity->header.id) {
		return WOTB_PLAYER;
	} else {
		assert(player_in_possession_entity);

		enum team_side player_team_side = player_entity->player.team_side;
		if (player_in_possession_entity->player.team_side == player_team_side) {
			return WOTB_TEAMMATE;
		} else {
			return WOTB_OPPONENT;
		}
	}
}

v2 calculate_position_to_seek_to_reach_ball(struct entity *player_entity,
											struct entity *ball_entity,
											struct game_state *game_state)
{
	// Let's move towards the ball
	struct player_ball_reachability_prediction prediction =
		entity_player_can_reach_future_ball_position(player_entity,
													 game_state->constants.player_run_max_velocity,
													 ball_entity->ball.lookahead_positions,
													 NULL,
													 game_state);
	v3 ball_future_pos;
	if (prediction.can_reach_it)
		ball_future_pos = prediction.future_pos;
	else
		ball_future_pos =
			V2_TO_V3(ball_entity->transform.position, ball_entity->transform.virtual_height);
	return V3_TO_V2(ball_future_pos);
}
