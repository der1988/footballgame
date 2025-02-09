#include "ai.h"
#include "e_ball.h"
#include "e_player.h"
#include "entities.h"
#include "football.h"
#include "math_ext.h"
#include "types.h"
#include "game.h"
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
									 struct game_state *game_state)
{
	enum player_ai_state ai_state;

	struct entity *ball_entity =
		entities_entity_get(game_state->entity_repository, game_state->vars.ball_entity_id);
	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
	UNUSED(ball_collider);
	UNUSED(ball_physics_3d);

	DECLARE_ENTITY_COMPONENT(player_entity, transform);

	b32 ball_in_keeper_hands = ball->state_machine->current_state == BS_GK_HANDS;

	entity_id_t player_in_possession_entity_id =
		entities_ball_player_in_possession(ball_entity, game_state);
	struct entity *player_in_possession_entity =
		entities_entity_get(game_state->entity_repository, player_in_possession_entity_id);

	if (ball_in_keeper_hands) {
		ai_state = PAS_KEEP_POSITION;
	} else {
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
			v2 goal_center = game_state->vars.pitch
								 .goal[game_state->vars.teams[1 - player_entity->player.team_side]
										   .play_context.pitch_side]
								 .center;
			f32 distance_from_goal =
				Vector2Distance(goal_center, player_entity->transform.position);
			if (distance_from_goal < 250) {
				v2 to_target_goal = Vector2Subtract(goal_center, player_entity->transform.position);
				f32 angle =
					fabs(Vector2Angle(player_entity->physics.velocity, to_target_goal) * RAD2DEG);
				if (angle < 30) {
					*kick_decision = PAKD_SHOOT;
					*kick_power = 0.5f + 0.5f * GetRandomValue(1, 100) / 100.f;
					v2 shoot_target = Vector2Zero();
					shoot_target.x += GetRandomValue(-60, 60) / 60.f;
					*target_position = shoot_target;
				} else {
					*target_position = Vector2Normalize(
						Vector2Subtract(goal_center, player_entity->transform.position));
				}
			} else {
				struct entity *opponent_entity =
					entities_player_find_nearest_facing_opponent(player_entity, game_state);
				struct entity *teammate_entity =
					entities_player_find_pass_target(player_entity, 60, 300, game_state);
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
				*target_position = calculate_position_to_seek_to_reach_ball(
					player_entity, ball_entity, game_state);
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
				ai_state = PAS_SEEK_POSITION;
				*target_position = ball_transform->position;
			} else {
				// We are not the nearest to the player with the ball, so let's mind our business
				// and move based on the tactics of the team
				ai_state = PAS_KEEP_POSITION;
			}
			break;
		}
		case WOTB_NO_ONE: {
			if (ball->pass_target_entity_id == player_entity->header.id) {
				// This player is the target of an automatic pass
				// Let's move towards the ball
				*target_position = calculate_position_to_seek_to_reach_ball(
					player_entity, ball_entity, game_state);
				ai_state = PAS_SEEK_POSITION;
			} else if (ball->future_bounce.available &&
					   Vector2Distance(transform->position, ball->future_bounce.position) < 50) {
				*target_position = ball->future_bounce.position;
				ai_state = PAS_ARRIVE_TO_POSITION;
			} else if (game_state->vars.teams[player_entity->player.team_side]
						   .play_context.player_nearest_to_the_ball == player_entity->header.id) {
				struct player_ball_reachability_prediction prediction =
					entities_player_can_reach_future_ball_position(
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
		entities_player_can_reach_future_ball_position(
			player_entity,
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
