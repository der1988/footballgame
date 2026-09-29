#include "macro.h"
#include "ai.h"
#include "collision.h"
#include "constants.h"
#include "entity_ball.h"
#include "entity_components.h"
#include "entity_player.h"
#include "entity.h"
#include "football.h"
#include "math_ext.h"
#include "game.h"
#include "physics.h"
#include "types.h"
#include <raymath.h>

struct goalie_ai_decision ai_goalie_think(struct entity *goalie_entity,
										  struct game_state *game_state)
{
	const struct component_player *player = &goalie_entity->player;
	const enum pitch_side pitch_side =
		game_state->vars.teams[player->team_side].play_context.pitch_side;

	switch (goalie_entity->goalie.state_machine.current_state) {
	case GS_PROTECT_GOAL: {
		struct entity *ball_entity = game_state->vars.ball.entity;
		DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
		UNUSED(ball_collider);

		const v2 goal_center = game_state->vars.pitch.goal[pitch_side].center;
		const f32 goal_width = game_state->vars.pitch.goal_width;

		const f32 ball_velocity = Vector3Length(ball_physics_3d->velocity);

		const entity_id_t player_in_possession_entity_id =
			entity_ball_player_in_possession(ball_entity, game_state).entity_id;

		// A ball a team mate played back to him has to be played with his feet
		const b32 may_handle_ball =
			!entity_ball_is_goalie_hands_barred(ball_entity, player->team_side);

		struct goalie_ai_decision decision = {.state = GAS_NONE, .action = GAA_NO_ACTION};

		const f32 exit_max_speed = player->constants.run_max_velocity;

		const struct future_position *future_reachable_position =
			&ball->future_positions[FUTURE_POSITION_REACHABLE_TO_GOALIE];

		// can we get the ball in our hands without moving by any chance?
		if (may_handle_ball &&
			ball_transform->virtual_height > game_state->constants.goalie_max_reachable_height &&
			future_reachable_position->available && future_reachable_position->dts < 20 &&
			Vector2Distance(V3_TO_V2(future_reachable_position->position),
							goalie_entity->transform.position) < 5) {
			decision.action = GAA_JUMP;
			return decision;
		}

		// Ball future reachable position is different from current position only in case the ball is currently
		// not reachable by players because it's floating high in the air.
		v2 ball_future_reachable_position = V3_TO_V2(future_reachable_position->position);
		struct nearest_player nearest_opponent_to_the_ball = {0};
		const b32 found = entity_player_find_nearest(ball_future_reachable_position,
													 OPPONENT_TEAM_SIDE(player->team_side),
													 LIST_ENTITY_IDS_EMPTY,
													 &nearest_opponent_to_the_ball,
													 game_state);

		b32 protect_goal = false;
		if ((future_reachable_position->available &&
			 football_pitch_is_position_inside_penalty_area(
				 ball_future_reachable_position, pitch_side, game_state)) &&
			player_in_possession_entity_id == EID_NOT_VALID &&
			ball_velocity < (15 * PIXELS_PER_METER) &&
			(!found || nearest_opponent_to_the_ball.distance > 10)) {
			// We will chase the ball if all the following conditions are true:
			// - nearest opponent is reasonably far from the ball
			// - no player in possession of the ball
			// - ball is in the penalty box or ball future bounce is in the penalty box
			// - ball velocity is low

			// Calculate whether goalie can reach the ball at its next reachable position
			const b32 should_seek_ball_at_reachable_pos =
				future_reachable_position->available &&
				physics_can_reach_position_in_time(goalie_entity->transform.position,
												   ball_future_reachable_position,
												   game_state->dt * future_reachable_position->dts,
												   exit_max_speed);

			if (should_seek_ball_at_reachable_pos) {
				// seek ball future reachable position
				decision.state = GAS_CLAIM_BALL;
				decision.steering = entity_player_steer_arrive(
					goalie_entity, ball_future_reachable_position, game_state);
			} else {
				// Calculate whether goalie can reach the ball using predicted positions
				// At this point we already know no one is in possession
				const struct player_ball_reachability_prediction prediction =
					entity_player_can_reach_future_ball_position(
						goalie_entity,
						exit_max_speed,
						game_state->vars.ball.lookahead_positions,
						entity_player_goalie_can_claim_ball_at,
						game_state);

				if (prediction.can_reach_it) {
					decision.state = GAS_CLAIM_BALL;
					decision.steering = entity_player_steer_arrive(
						goalie_entity, V3_TO_V2(prediction.future_pos), game_state);
				} else {
					// ball isn't within reach, let's protect goal
					protect_goal = true;
				}
			}
		} else {
			// Protect goal
			protect_goal = true;
		}

		if (protect_goal) {
			// Let's understand if we have to prepare for a dive
			b32 move_on_the_line = false;

			// We need to account for both pitch sides
			const b32 invert = pitch_side == PSI_NORTH;

			const f32 max_side_movement = goal_width * 0.5f * 0.7f;
			v2 left_pos =
				Vector2Add(goal_center, (v2){.x = -max_side_movement, .y = 20 * (invert ? 1 : -1)});
			v2 right_pos =
				Vector2Add(goal_center, (v2){.x = max_side_movement, .y = 20 * (invert ? 1 : -1)});

			for (u16 i = 0; i < game_state->constants.ball_lookaheads; ++i) {
				const v3 pos_to_check = game_state->vars.ball.lookahead_positions[i];

				v2 prev_pos;
				v2 new_pos;
				if (i == 0) {
					prev_pos = ball_transform->position;
					new_pos = V3_TO_V2(pos_to_check);
				} else {
					prev_pos = V3_TO_V2(game_state->vars.ball.lookahead_positions[i - 1]);
					new_pos = V3_TO_V2(pos_to_check);
				}

				v2 intersection_point;
				if (collision_is_line_intersecting_line(
						prev_pos, new_pos, left_pos, right_pos, &intersection_point)) {
					decision.state = GAS_COVER_LINE;
					decision.steering = entity_player_steer_arrive_limited(
						goalie_entity,
						intersection_point,
						player->constants.run_max_velocity * 0.5f,
						game_state);
					move_on_the_line = true;
					break;
				}
			}

			if (!move_on_the_line) {
				v2 direction =
					Vector2Normalize(Vector2Subtract(ball_transform->position, goal_center));
				decision.state = GAS_STAND;
				decision.steering =
					entity_player_steer_arrive(goalie_entity,
											   Vector2Add(goal_center, Vector2Scale(direction, 20)),
											   game_state);
			}
		}

		// Only dive if ball velocity is higher than 36km/h
		if (may_handle_ball &&
			ball_velocity > game_state->constants.goalie_dive_min_ball_velocity &&
			entity_player_goalie_should_dive(
				goalie_entity, ball_entity, &decision.action_data.dive, game_state)) {
			decision.action = GAA_DIVE;
		}

		return decision;
	}
	case GS_HOLD_BALL: {
		const enum team_side opposing_team = OPPONENT_TEAM_SIDE(player->team_side);

		// He looks up before doing anything, rather than acting on the tick he gathers it
		if (!HAS_TIME_PASSED(
				game_state->time,
				goalie_entity->goalie.state_machine.state_data.hold_ball.hold_start_time,
				GOALIE_AI_THINK_TIME)) {
			return (struct goalie_ai_decision){.state = GAS_NONE, .action = GAA_NO_ACTION};
		}

		struct nearest_player nearest_opponent = {0};
		const b32 is_opponent_found = entity_player_find_nearest(goalie_entity->transform.position,
																 opposing_team,
																 LIST_ENTITY_IDS_EMPTY,
																 &nearest_opponent,
																 game_state);

		// Closed down, so he gets rid of it
		if (is_opponent_found && nearest_opponent.distance < GOALIE_AI_PRESSURE_RADIUS) {
			return (struct goalie_ai_decision){
				.state = GAS_NONE, .action = GAA_KICK, .action_data.kick = {.power = 1.0f}};
		}

		// A team mate he can reach by hand, with nobody in the way of it
		const v2 up_the_pitch = {.y = pitch_side == PSI_NORTH ? 1.0f : -1.0f};
		struct entity *throw_target;
		if (ai_pass_find_target(goalie_entity,
								up_the_pitch,
								GOALIE_AI_THROW_HALF_ANGLE,
								GOALIE_AI_THROW_RANGE * 0.6f,
								GOALIE_AI_THROW_RANGE,
								true,
								&throw_target,
								game_state)) {
			const f32 distance_to_target = Vector2Distance(goalie_entity->transform.position,
														   throw_target->transform.position);
			return (struct goalie_ai_decision){
				.state = GAS_NONE,
				.action = GAA_THROW,
				.action_data.throw_ball = {
					.power = Clamp(distance_to_target / GOALIE_AI_THROW_RANGE, 0.2f, 1.0f),
					.target_entity_id = throw_target->header.id}};
		}

		// Nobody near him at all, so he sets it down and plays out
		if (!is_opponent_found || nearest_opponent.distance > GOALIE_AI_PLAY_OUT_RADIUS) {
			return (struct goalie_ai_decision){.state = GAS_NONE, .action = GAA_PUT_DOWN};
		}

		// Otherwise he walks out towards the edge of his area, where all of this is asked again
		const struct goal *goal = &game_state->vars.pitch.goal[pitch_side];
		const v2 area_edge = {.x = goal->penalty_box.x + goal->penalty_box.width * 0.5f,
							  .y = pitch_side == PSI_NORTH ?
									   goal->penalty_box.y + goal->penalty_box.height :
									   goal->penalty_box.y};

		return (struct goalie_ai_decision){
			.state = GAS_CARRY,
			.steering = entity_player_steer_arrive(goalie_entity, area_edge, game_state),
			.action = GAA_NO_ACTION};
	}
	case GS_DIVE:
	case GS_JUMP:
	case GS_THROW:
	case GS_NONE:
	case GS_COUNT:
		break;
	}
	return (struct goalie_ai_decision){.state = GAS_NONE, .action = GAA_NO_ACTION};
}
