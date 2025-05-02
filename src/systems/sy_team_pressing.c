#include "components.h"
#include "entity_ball.h"
#include "entity_player.h"
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

struct entity *entities_player_select_press_player(struct entity *ball_entity,
												   enum team_side team_side,
												   f32 max_distance,
												   struct game_state *game_state)
{
	struct entity_repository *entity_repository = game_state->entity_repository;
	struct entity *selected_entity = NULL;
	f32 selected_score = 0;
	struct entity *entity;

	v3 *ball_future_positions = ball_entity->ball.lookahead_positions;

	for (usize entity_id = 0; entity_id < entity_repository->max_slots; ++entity_id) {
		entity = entity_get(game_state->entity_repository, entity_id);

		if (!entity->header.alive || entity->header.type != ET_PLAYER ||
			entity->player.state_machine->current_state != PS_NORMAL ||
			entity->player.team_side != team_side ||
			entity_id ==
				game_state->vars.teams[team_side].play_context.player_nearest_to_the_ball) {
			continue;
		}

		b32 is_player_human_controlled = false;
		for (usize i = 0; i < MAX_HUMAN_PLAYERS; ++i) {
			if (game_state->human_players[i].controlled_player_entity_id == entity_id) {
				is_player_human_controlled = true;
				break;
			}
		}
		if (is_player_human_controlled) {
			continue;
		}

		struct component_transform *transform = &entity->transform;

		// Calculate future positions score
		f32 best_future_position_score = 0;
		for (int i = 0; i < BALL_MAX_LOOKAHEADS; i++) {
			v2 future_ball_pos = V3_TO_V2(ball_future_positions[i]);
			f32 future_distance_sqr = Vector2DistanceSqr(transform->position, future_ball_pos);
			f32 future_score = 1 - Clamp01(future_distance_sqr / (max_distance * max_distance));

			// Weight earlier predictions more heavily
			f32 time_weight = 1.0f - ((f32)i / BALL_MAX_LOOKAHEADS);
			future_score *= time_weight;

			if (future_score > best_future_position_score) {
				best_future_position_score = future_score;
			}
		}

		if (best_future_position_score > selected_score) {
			selected_entity = entity;
			selected_score = best_future_position_score;
		}
	}
	if (selected_entity != NULL) {
		game_state->vars.teams[team_side].play_context.player_in_charge_of_pressing =
			selected_entity->header.id;
		GLOGF_TRACE(LS_GAME,
					"selected press player %zu with score %.2f",
					selected_entity->header.id,
					selected_score);
	}
	return selected_entity;
}

void systems_team_pressing(struct game_state *game_state)
{
	struct entity *ball_entity = game_state->vars.ball_entity;

	entity_id_t player_in_possession_entity_id = ball_player_in_possession(ball_entity, game_state);
	struct entity *player_in_possession_entity =
		entity_get(game_state->entity_repository, player_in_possession_entity_id);
	if (player_in_possession_entity) {
		enum team_side team_in_possession = player_in_possession_entity->player.team_side;
		entities_player_select_press_player(ball_entity, 1 - team_in_possession, 250.f, game_state);
	} else {
		for (usize i = 0; i < TS_COUNT; ++i) {
			entities_player_select_press_player(ball_entity, i, 250.f, game_state);
		}
	}
}
