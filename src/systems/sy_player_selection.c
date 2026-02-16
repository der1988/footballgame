#include "components.h"
#include "e_ball.h"
#include "e_player.h"
#include "entities.h"
#include "football.h"
#include "game.h"
#include "input.h"
#include "macro.h"
#include "math_ext.h"
#include "types.h"
#include "assert.h"
#include "ai.h"
#include <float.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum ball_possession {
	BALL_POSSESSION_NONE,
	BALL_POSSESSION_TEAM,
	BALL_POSSESSION_OPPO,
};

struct human_info {
	usize human_player_idx;
	f32 controlled_player_distance_from_ball;
};

struct list_human_players {
	struct human_info *array;
	usize count;
	usize size;
};

int compare_distances(const void *a, const void *b)
{
	const struct human_info *human_info_a = (const struct human_info *)a;
	const struct human_info *human_info_b = (const struct human_info *)b;

	if (human_info_a->controlled_player_distance_from_ball <
		human_info_b->controlled_player_distance_from_ball)
		return -1;
	if (human_info_a->controlled_player_distance_from_ball >
		human_info_b->controlled_player_distance_from_ball)
		return 1;
	return 0;
}

struct list_human_players order_humans_by_distance_from_ball(struct human_player *human_players,
															 enum team_side team_side,
															 v2 ball_position,
															 memory_arena_handle_t *memory_arena,
															 struct game_state *game_state)
{
	LIST_CREATE(struct list_human_players, sorted_human_players, MAX_HUMAN_PLAYERS, memory_arena);

	usize total = 0;
	for (usize i = 0; i < MAX_HUMAN_PLAYERS; ++i) {
		struct human_player *human_player = &human_players[i];

		if (human_player->enabled && team_side == human_player->team_side) {
			struct entity *player_entity = entity_get(game_state->entity_repository,
													  human_player->controlled_player_entity_id);
			f32 distance = FLT_MAX;
			if (player_entity) {
				distance = Vector2DistanceSqr(ball_position, player_entity->transform.position);
			}
			struct human_info hi = {.human_player_idx = i,
									.controlled_player_distance_from_ball = distance};
			LIST_ADD(sorted_human_players, hi);
			total++;
		}
	}

	qsort(sorted_human_players.array, total, sizeof(struct human_info), compare_distances);

	return sorted_human_players;
}

void systems_player_selection(struct game_state *game_state, struct input *input)
{
	UNUSED(input);

	struct entity *ball_entity = game_state->vars.ball_entity;

	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
	UNUSED(ball_physics_3d);
	UNUSED(ball_collider);
	UNUSED(ball_transform);

	// Let's use the future reachable position of the ball to determine which player to select
	// This is useful in particular for crosses and long passes.
	// If the ball is on the ground the future reachable position is its current position.
	v2 epicenter = ball_transform->position;
	struct future_position *future_position_reachable_to_player =
		&ball->future_positions[FUTURE_POSITION_REACHABLE_TO_PLAYER];
	if (future_position_reachable_to_player->available) {
		epicenter = V3_TO_V2(future_position_reachable_to_player->position);
	}

	entity_id_t player_in_possession_entity_id =
		entity_ball_player_in_possession(ball_entity, game_state);
	struct entity *player_in_possession_entity =
		entity_get(game_state->entity_repository, player_in_possession_entity_id);

	// Process one team at a time
	for (usize team_side_idx = 0; team_side_idx < TS_COUNT; ++team_side_idx) {

		// Let's determine if this team is in possession of the ball
		enum ball_possession ball_possession = BALL_POSSESSION_NONE;
		if (player_in_possession_entity) {
			ball_possession = player_in_possession_entity->player.team_side == team_side_idx ?
								  BALL_POSSESSION_TEAM :
								  BALL_POSSESSION_OPPO;
		}

		b32 is_player_in_possession_controlled_by_human = false;
		for (usize i = 0; i < MAX_HUMAN_PLAYERS; ++i) {
			struct human_player *human_player = &game_state->human_players[i];

			if (human_player->enabled && ball_possession == BALL_POSSESSION_TEAM &&
				human_player->team_side == team_side_idx &&
				human_player->controlled_player_entity_id != EID_NOT_VALID &&
				human_player->controlled_player_entity_id == player_in_possession_entity_id) {
				is_player_in_possession_controlled_by_human = true;
				break;
			}
		}

		LIST_CREATE(struct list_entity_ids,
					excluded_players_list,
					10,
					game_state->memory_by_lifetime[MAL_UPDATE]);

		LIST_CREATE(struct list_entity_ids,
					tmp_excluded_players_list,
					20,
					game_state->memory_by_lifetime[MAL_UPDATE]);

		// We don't want to select again the same player that just kicked the ball.
		// Let's not bother to check whether kicker is in our team or not, we don't care.
		if (!HAS_TIME_PASSED(game_state->time, ball->last_kick_time, 0.5f) &&
			ball->last_kicker_entity_id != EID_NOT_VALID) {
			LIST_ADD(excluded_players_list, ball->last_kicker_entity_id);
			is_player_in_possession_controlled_by_human = true;
		}

		struct list_human_players ordered_list =
			order_humans_by_distance_from_ball(game_state->human_players,
											   team_side_idx,
											   epicenter,
											   game_state->memory_by_lifetime[MAL_UPDATE],
											   game_state);

		for (usize i = 0; i < ordered_list.count; ++i) {
			// Build list of other players currently controlled by humans in this team, to avoid stepping on their shoes
			// We do this at every iteration because this list might change.
			LIST_RESET(tmp_excluded_players_list);
			for (usize j = 0; j < MAX_HUMAN_PLAYERS; ++j) {
				struct human_player *other_human_player = &game_state->human_players[j];

				if (other_human_player->enabled && other_human_player->team_side == team_side_idx &&
					j != ordered_list.array[i].human_player_idx &&
					other_human_player->controlled_player_entity_id != EID_NOT_VALID) {
					LIST_ADD(tmp_excluded_players_list,
							 other_human_player->controlled_player_entity_id);
				}
			}

			struct human_player *human_player =
				&game_state->human_players[ordered_list.array[i].human_player_idx];

			if (ball_possession == BALL_POSSESSION_TEAM) {
				if (human_player->controlled_player_entity_id ==
					player_in_possession_entity->header.id) {
					if (!player_in_possession_entity->player.kicked_ball) {
						// If the currently controlled player is in possession of the ball (and they didn't kick it in this frame) we just select the same player again.
						continue;
					} else {
						// If the currently controlled player was in possession of the ball but then they kicked it during this frame, we need to exclude current player and select a new one.
						LIST_ADD(tmp_excluded_players_list,
								 human_player->controlled_player_entity_id);
					}
				} else {
					// If no human is controlling player in possession we assign it to the first human player
					if (!is_player_in_possession_controlled_by_human &&
						human_player->team_side == team_side_idx &&
						!player_in_possession_entity->player.kicked_ball) {
						human_player->controlled_player_entity_id = player_in_possession_entity_id;
						is_player_in_possession_controlled_by_human = true;
						continue;
					}
				}
			}

			// Add team wide excluded players to the temporary list of this iteration
			LIST_ITERATE(excluded_player, excluded_players_list)
			{
				if (excluded_player != human_player->controlled_player_entity_id) {
					LIST_ADD(tmp_excluded_players_list, excluded_player);
				}
			}

			// Find player nearest to the epicenter
			struct nearest_player *nearest_player = find_nearest_player(
				epicenter, human_player->team_side, tmp_excluded_players_list, game_state);
			// nearest_player is always valid
			entity_id_t new_controlled_player_entity_id = nearest_player->entity->header.id;

			if (new_controlled_player_entity_id != EID_NOT_VALID &&
				new_controlled_player_entity_id != human_player->controlled_player_entity_id) {
				// human is controlling a new player compared to previous frame
				game_controller_reset(
					&game_state->input_manager->game_controllers[human_player->game_controller_id]);
				human_player->controlled_player_entity_id = new_controlled_player_entity_id;
			}
		}
	}

	// for (usize i = 0; i < MAX_HUMAN_PLAYERS; ++i) {
	// 	struct human_player *human_player = &game_state->human_players[i];

	// 	if (human_player->enabled) {
	// 		printf("human player %d is controlling entity %d\n",
	// 			   human_player->id,
	// 			   human_player->controlled_player_entity_id);
	// 	}
	// }
}
