#include "macro.h"
#include "ai.h"
#include "constants.h"
#include "entity_components.h"
#include "entity.h"
#include "entity_player.h"
#include "football.h"
#include "game.h"
#include "log.h"
#include "math_ext.h"
#include "types.h"
#include <raymath.h>

b32 ai_pass_find_target(struct entity *passer_entity,
						v2 scan_direction,
						f32 detection_angle,
						f32 target_distance,
						f32 max_distance,
						b32 require_clear_trajectory,
						struct entity **out_pass_target,
						struct game_state *game_state)
{
	const enum pitch_side pitch_side =
		game_state->vars.teams[passer_entity->player.team_side].play_context.pitch_side;
	const struct component_ball *ball = &game_state->vars.ball.entity->ball;

	struct entity *selected_entity = NULL;
	f32 selected_score = 0;
	FOR_EACH_ENTITY(candidate_entity, game_state->entity_repository)
	{
		if (!candidate_entity->header.alive || candidate_entity->header.type != ET_PLAYER ||
			candidate_entity->header.id == passer_entity->header.id ||
			candidate_entity->player.team_side != passer_entity->player.team_side) {
			continue;
		}

		v2 to_player = Vector2Subtract(candidate_entity->transform.position,
									   passer_entity->transform.position);
		f32 angle = fabs(Vector2Angle(scan_direction, to_player) * RAD2DEG);
		const f32 distance = Vector2Length(to_player);
		if (angle > detection_angle || distance > max_distance)
			continue;

		b32 nearer_to_goal =
			pitch_side == PSI_NORTH ?
				candidate_entity->transform.position.y > passer_entity->transform.position.y :
				candidate_entity->transform.position.y < passer_entity->transform.position.y;

		b32 clear_trajectory = entity_player_is_trajectory_clear(
			passer_entity->transform.position,
			candidate_entity->transform.position,
			OPPONENT_TEAM_SIDE(candidate_entity->player.team_side),
			PASS_INTERCEPTION_RADIUS,
			game_state);
		if (require_clear_trajectory && !clear_trajectory)
			continue;

		// The man nearest to target_distance along his own line from the passer ranks highest
		const f32 distance_from_target = fabsf(distance - target_distance);
		f32 distance_factor =
			Clamp01((distance_from_target * distance_from_target) / (max_distance * max_distance));
		f32 score = 0.15f * (1 - distance_factor);
		score += 0.15f * nearer_to_goal;
		score += 0.2f * clear_trajectory;

		if (candidate_entity->header.id == ball->last_play_entity_id) {
			const f32 time_since_play = game_state->time - ball->last_play_time;
			score -= AI_PASS_BACK_PENALTY * (1 - Clamp01(time_since_play / AI_PASS_BACK_TIME));
		}

		if (score > selected_score) {
			selected_entity = candidate_entity;
			selected_score = score;
		}
	}
	if (selected_entity) {
		GLOGF_TRACE(LS_GAME,
					"AI selected pass target %u with score %.2f",
					selected_entity->header.id,
					selected_score);
		*out_pass_target = selected_entity;
	}
	return selected_entity != NULL;
}
