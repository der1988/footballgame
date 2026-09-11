#include "macro.h"
#include "ai.h"
#include "entity_components.h"
#include "entity_ball.h"
#include "entity_player.h"
#include "entity.h"
#include "football.h"
#include "math_ext.h"
#include "sprite.h"
#include "types.h"
#include "game.h"
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>

#define STAY_THERE_RADIUS 30.0f

// Per tick, at the fixed 480Hz step, so about a fifth of a second of carrying it on average
// once he has decided the ball should move.
#define AI_PASS_PROBABILITY_PER_TICK 0.01f

enum whos_on_the_ball {
	WOTB_PLAYER,
	WOTB_TEAMMATE,
	WOTB_OPPONENT,
	WOTB_NO_ONE
};

static enum whos_on_the_ball identify_whos_in_possession(
	struct entity *player_entity, struct entity *player_in_possession_entity);

static b32 team_has_human_players(enum team_side team_side, struct game_state *game_state);

enum player_ai_state ai_player_think(struct entity *player_entity,
									 v2 *target_position,
									 enum player_ai_action_decision *action_decision,
									 f32 *power,
									 struct game_state *game_state)
{
	enum player_ai_state ai_state;

	struct entity *ball_entity = game_state->vars.ball.entity;
	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
	UNUSED(ball_collider);
	UNUSED(ball_physics_3d);

	DECLARE_ENTITY_COMPONENT(player_entity, transform);

	// Initialise all out-params before any early return: several code paths
	// (e.g. the FS_PLAY / keeper-hands guard below) return without deciding an
	// action, and the caller reads action_decision unconditionally. Leaving it
	// uninitialised is UB that surfaces under -O2 as spurious headers.
	*action_decision = PAAD_NO_ACTION;
	*power = 0.0f;
	*target_position = transform->position;

	b32 ball_in_keeper_hands = ball->state_machine.current_state == BS_GK_HANDS;

	entity_id_t player_on_the_ball_entity_id =
		entity_ball_player_on_the_ball(ball_entity, game_state);
	entity_id_t player_in_possession_entity_id =
		entity_ball_player_in_possession(ball_entity, game_state);
	struct entity *player_in_possession_entity =
		entity_get(game_state->entity_repository, player_in_possession_entity_id);

	const b32 team_has_humans = team_has_human_players(player_entity->player.team_side, game_state);
	if (game_state->gameplay.state_machine.current_state != FS_PLAY || ball_in_keeper_hands) {
		return PAS_KEEP_POSITION;
	}

	enum whos_on_the_ball whos_on_the_ball =
		player_in_possession_entity_id == EID_NOT_VALID ?
			WOTB_NO_ONE :
			identify_whos_in_possession(player_entity, player_in_possession_entity);

	switch (whos_on_the_ball) {
	case WOTB_PLAYER: {
		// We possess the ball

		// If the player who has possession of the ball is controlled by AI, but the team has a human player,
		// then we should really hand over control to the human
		if (team_has_humans) {
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

			v2 to_target_goal = Vector2Subtract(goal_center, player_entity->transform.position);
			f32 angle =
				fabs(Vector2Angle(player_entity->physics.velocity, to_target_goal) * RAD2DEG);
			if (angle < 30) {
				*action_decision = PAAD_SHOOT;
				*power = 0.5f + 0.5f * RANDOM_01(100);

				if (game_state->constants.shoot_aid_auto_aim) {
					v2 shoot_target = Vector2Zero();
					shoot_target.x += GetRandomValue(-60, 60) / 60.f;
					*target_position = shoot_target;
				} else {
					f32 accuracy =
						40 +
						GetRandomValue(
							0, 20); // TODO: this should really depend on ai level and player stats

					v2 shot_direction_to_center =
						Vector2Normalize(Vector2Subtract(goal_center, ball_transform->position));
					v2 shot_direction_to_right_post = Vector2Normalize(Vector2Subtract(
						Vector2Add(goal_center, (v2){.x = accuracy}), ball_transform->position));
					f32 angle_center_to_post =
						Vector2Angle(shot_direction_to_center, shot_direction_to_right_post);

					v2 shot_direction =
						Vector2Rotate(shot_direction_to_center,
									  angle_center_to_post * GetRandomValue(-100, 100) / 100.f);

					*target_position = shot_direction;
				}
			} else {
				*target_position = Vector2Normalize(
					Vector2Subtract(goal_center, player_entity->transform.position));
			}
		} else {
			struct nearest_player nearest_facing_oppo = {0};
			b32 nearest_facing_oppo_found = entity_player_find_nearest_facing_opponent(
				player_entity, &nearest_facing_oppo, game_state);
			struct entity *teammate_entity;
			const b32 pass_target_found = entity_player_find_pass_target(
				player_entity,
				ORIENTATION_VECTORS[player_entity->sprite.orientation],
				90,
				0,
				game_state->constants.ball_pass_auto_max_distance,
				&teammate_entity,
				game_state);
			b32 passing = false;
			if (pass_target_found && nearest_facing_oppo_found) {
				// He has decided the ball should move, but not on the same tick every time:
				// rolling for it spreads the release out instead of firing it the instant the
				// conditions line up.
				passing = RANDOM_01(100000) < AI_PASS_PROBABILITY_PER_TICK;
			}

			if (passing) {
				*action_decision = PAAD_PASS;
				*power = 0.1f;
				*target_position = Vector2Normalize(Vector2Subtract(
					teammate_entity->transform.position, player_entity->transform.position));
			} else {
				*action_decision = PAAD_NO_ACTION;
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
			*target_position = entity_player_position_to_seek_to_reach_ball(
				player_entity, ball_entity, game_state);
			ai_state = PAS_SEEK_POSITION;
		} else {
			ai_state = PAS_KEEP_POSITION;
		}
		break;
	}
	case WOTB_OPPONENT: {
		// Closing down an opponent who has the ball is the presser's job. A pure AI team sends the
		// player nearest to the ball along with him; a team with humans doesn't, so that when the
		// human isn't asking to press (no presser elected) nobody chases.
		if (game_state->vars.teams[player_entity->player.team_side]
					.play_context.player_in_charge_of_pressing == player_entity->header.id ||
			(!team_has_humans &&
			 game_state->vars.teams[player_entity->player.team_side]
					 .play_context.player_nearest_to_the_ball == player_entity->header.id)) {
			// We are the nearest to the player with the ball, let's seek the ball and potentially tackle it
			ai_state = PAS_SET_STEERING;
			*target_position = entity_player_steer_pursuit(
				player_entity, ball_transform->position, V3_TO_V2(ball_physics_3d->velocity));

			if (player_in_possession_entity) {
				// Close enough to play the ball while the opponent still holds it, so go in for
				// it. Sending a man to press is asking him to win the ball, so he goes in
				// whoever sent him - a human holding the press button included.
				if (player_entity->player.is_colliding_with_ball) {
					*action_decision = PAAD_TACKLE;
					break;
				}

				// The slide stays with fully AI teams. It is the lunge at what he cannot reach,
				// and a human sending a teammate to press has not asked him to gamble a card on
				// it.
				f32 dist_to_ball =
					Vector2Distance(player_entity->transform.position, ball_transform->position);
				if (!team_has_humans && dist_to_ball < 20.0f) {
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
					// The angle is 0 with the tackler stood in the carrier's path and 180 with him
					// chasing from directly behind, which is the one that gets a card. Minimum
					// 0.05% probability per tick from behind, maximum 0.75% going in facing him.
					f32 tackle_probability = 0.0005f + (1.0f - angle / 180.0f) * 0.007f;
					if (RANDOM_01(100000) < tackle_probability) {
						*action_decision = PAAD_SLIDING_TACKLE;
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
			*target_position = entity_player_position_to_seek_to_reach_ball(
				player_entity, ball_entity, game_state);
			ai_state = PAS_ARRIVE_TO_POSITION;
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
					player_entity->player.constants.run_max_velocity,
					game_state->vars.ball.lookahead_positions,
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

		*action_decision = PAAD_NO_ACTION;

		// Header attempt - only when ball is not high, team is fully AI controlled and enough passed from previous attempt
		if (!team_has_humans &&
			HAS_TIME_PASSED(game_state->time, player_entity->player.last_header_time, 0.5f)) {
			if (!player_entity->player.is_colliding_with_ball &&
				ball_transform->virtual_height > HEADER_MIN_HEIGHT &&
				ball_transform->virtual_height < HEADER_MAX_HEIGHT &&
				Vector2Distance(transform->position, ball_transform->position) < 50.0f) {

				v2 goal_center =
					game_state->vars.pitch
						.goal[game_state->vars.teams[1 - player_entity->player.team_side]
								  .play_context.pitch_side]
						.center;
				f32 distance_from_goal =
					Vector2Distance(goal_center, player_entity->transform.position);

				*action_decision = distance_from_goal < 250 ? PAAD_HEADER_SHOOT : PAAD_HEADER_PASS;
			}
		}

		break;
	}
	}

	return ai_state;
}

static enum whos_on_the_ball identify_whos_in_possession(struct entity *player_entity,
														 struct entity *player_in_possession_entity)
{
	if (player_entity->header.id == player_in_possession_entity->header.id) {
		return WOTB_PLAYER;
	} else {
		ASSERT(player_in_possession_entity);

		enum team_side player_team_side = player_entity->player.team_side;
		if (player_in_possession_entity->player.team_side == player_team_side) {
			return WOTB_TEAMMATE;
		} else {
			return WOTB_OPPONENT;
		}
	}
}

static b32 team_has_human_players(enum team_side team_side, struct game_state *game_state)
{
	for (usize i = 0; i < MAX_HUMAN_PLAYERS; ++i) {
		if (game_state->human_players[i].enabled &&
			game_state->human_players[i].team_side == team_side) {
			return true;
		}
	}
	return false;
}
