#include "system.h"
#include "entity_components.h"
#include "entity_ball.h"
#include "entity_player.h"
#include "entity.h"
#include "football.h"
#include "game.h"
#include "input.h"
#include "macro.h"
#include "math_ext.h"
#include "types.h"
#include <assert.h>
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

static int compare_distances(const void *a, const void *b);

static struct list_human_players
order_humans_by_distance_from_ball(struct human_player *human_players,
								   enum team_side team_side,
								   v2 ball_position,
								   memory_arena_handle_t *memory_arena,
								   struct game_state *game_state);

static void switch_to_player(entity_id_t switch_to,
							 struct human_player *human_player,
							 struct game_state *game_state);

void system_player_selection(struct game_state *game_state)
{
	struct entity *ball_entity = game_state->vars.ball.entity;

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
		const usize team_human_players_count =
			game_human_players_count_by_team(game_state, team_side_idx);
		if (team_human_players_count == 0) {
			// No point in running this logic for pure AI team
			continue;
		}

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
				human_player->controlled_player.entity_id != EID_NOT_VALID &&
				human_player->controlled_player.entity_id == player_in_possession_entity_id) {
				is_player_in_possession_controlled_by_human = true;
				break;
			}
		}

		LIST_CREATE(struct list_entity_ids,
					team_excluded_players_list,
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
			LIST_ADD(team_excluded_players_list, ball->last_kicker_entity_id);
		}

		if (game_state->constants.player_pass_target_not_selectable &&
			ball->pass_target_entity_id != EID_NOT_VALID) {
			// We don't want to select the target of an automatic pass until he gets the ball
			LIST_ADD(team_excluded_players_list, ball->pass_target_entity_id);
		}

		struct list_human_players ordered_list =
			order_humans_by_distance_from_ball(game_state->human_players,
											   team_side_idx,
											   epicenter,
											   game_state->memory_by_lifetime[MAL_UPDATE],
											   game_state);

		// Iterate in the following order:
		// not selected -> farthest -> nearest
		for (size i = ordered_list.count - 1; i >= 0; --i) {
			struct human_player *human_player =
				&game_state->human_players[ordered_list.array[i].human_player_idx];

			b32 force_switch = false;

			struct game_controller *game_controller =
				&game_state->input_manager->game_controllers[human_player->game_controller_id];
			struct game_controller_state *game_controller_state = &game_controller->state;

			const b32 is_human_required_player_switch =
				game_controller_state->current[GAME_CONTROL_SWITCH_PLAYER].digital.pressed;

			force_switch = is_human_required_player_switch;

			const b32 is_currently_selected_player_manually_selected =
				human_player->controlled_player.is_manually_selected;

			struct entity *currently_selected_player_entity = entity_get(
				game_state->entity_repository, human_player->controlled_player.entity_id);

			if (currently_selected_player_entity &&
				currently_selected_player_entity->player.state_machine.current_state == PS_FALL) {
				// Let's force a switch if we fell.
				force_switch = true;
			}

			if (!HAS_TIME_PASSED(game_state->time, ball->last_kick_time, 0.5f) &&
				ball->last_kicker_entity_id != EID_NOT_VALID &&
				currently_selected_player_entity->header.id == ball->last_kicker_entity_id) {
				// Let's force a switch if we kicked the ball
				force_switch = true;
			}

			// Human required player switch so we OR the flag as we don't want to reset it when the player releases the switch control.
			// It's going to be reset when the player gets the ball, or other cases still TBD.
			human_player->controlled_player.is_manually_selected |= is_human_required_player_switch;

			// Don't switch away from the kicker while after touch is active unless the human requested the switch
			if (ball->after_touch.active &&
				human_player->controlled_player.entity_id == ball->last_kicker_entity_id &&
				!HAS_TIME_PASSED(game_state->time,
								 ball->last_kick_time,
								 game_state->constants.ball_after_touch_max_time) &&
				!is_human_required_player_switch) {
				continue;
			}

			// Don't switch away from a player performing a header if he hasn't hit the ball yet
			{
				if (currently_selected_player_entity &&
					(currently_selected_player_entity->player.state_machine.current_state ==
						 PS_HEADER &&
					 !currently_selected_player_entity->player.state_machine.state_data.header
						  .hit_ball) &&
					!force_switch) {
					continue;
				}
			}

			// Build list of other players currently controlled by humans in this team, to avoid stepping on their shoes
			// We do this at every iteration because this list might change.
			LIST_RESET(tmp_excluded_players_list);
			for (usize j = 0; j < MAX_HUMAN_PLAYERS; ++j) {
				struct human_player *other_human_player = &game_state->human_players[j];

				if (other_human_player->enabled && other_human_player->team_side == team_side_idx &&
					(is_human_required_player_switch ||
					 j !=
						 ordered_list.array[i]
							 .human_player_idx) && // If human is requiring player switch we want to exclude currently selected player
					other_human_player->controlled_player.entity_id != EID_NOT_VALID) {
					LIST_ADD(tmp_excluded_players_list,
							 other_human_player->controlled_player.entity_id);
				}
			}

			// Following logic makes sense even if player is requiring switch
			if (ball_possession == BALL_POSSESSION_TEAM) {
				if (human_player->controlled_player.entity_id ==
					player_in_possession_entity->header.id) {
					// Current human player is in possession of the ball.
					// Note: entity_ball_player_in_possession requires last_touch_time > last_kick_time,
					// so kicked_ball can never be true here — if the player kicked in this frame,
					// last_touch_time == last_kick_time and possession is already NONE.
					// The kicked_ball case is handled separately below.
					human_player->controlled_player.is_manually_selected = false;
					continue;
				} else {
					// If no human is controlling player in possession we assign it to the first human player
					// But not if it's a goalie who isn't holding the ball (e.g. mid-dive save)
					b32 is_goalie_but_not_holding_ball =
						player_in_possession_entity->player.state_machine.current_state ==
							PS_GOALIE &&
						player_in_possession_entity->goalie.state_machine.current_state !=
							GS_HOLD_BALL;
					if (!is_player_in_possession_controlled_by_human &&
						human_player->team_side == team_side_idx &&
						!player_in_possession_entity->player.kicked_ball &&
						!is_goalie_but_not_holding_ball) {
						human_player->controlled_player.entity_id = player_in_possession_entity_id;
						is_player_in_possession_controlled_by_human = true;
						continue;
					}
				}
			}

			// If current player was manually selected and didn't kick the ball in this frame we can return early.
			// If we have to force a switch though we have to comply.
			if (is_currently_selected_player_manually_selected && !force_switch) {
				continue;
			}

			// Add team wide excluded players to the temporary list of this iteration
			assert(tmp_excluded_players_list.size >=
				   (tmp_excluded_players_list.count + team_excluded_players_list.count));
			memcpy(&tmp_excluded_players_list.array[tmp_excluded_players_list.count],
				   team_excluded_players_list.array,
				   sizeof(entity_id_t) * team_excluded_players_list.count);
			tmp_excluded_players_list.count += team_excluded_players_list.count;

			// This logic applies only if human required player switch in this frame
			if (is_human_required_player_switch) {
				// First we look for a player in the opposite direction of current steering, as the human
				// is probably pressing in the direction he wants his candidate player to move to.
				struct game_controller *game_controller =
					&game_state->input_manager->game_controllers[human_player->game_controller_id];

				v2 scan_direction = Vector2Negate(input_movement_vector(game_controller));

				struct entity *switch_target;
				const b32 switch_target_found = entity_player_find_switch_target(
					ball_transform->position,
					scan_direction,
					45,
					300,
					human_player->team_side,
					currently_selected_player_entity ? currently_selected_player_entity->header.id :
													   EID_NOT_VALID,
					tmp_excluded_players_list,
					&switch_target,
					game_state);
				if (switch_target_found) {
					entity_id_t new_controlled_player_id = switch_target->header.id;
					if (new_controlled_player_id != EID_NOT_VALID &&
						new_controlled_player_id != human_player->controlled_player.entity_id) {
						// human is controlling a new player compared to previous frame
						switch_to_player(new_controlled_player_id, human_player, game_state);
						return;
					}
				}
			}
			// Find player nearest to the epicenter
			struct nearest_player nearest_player = {0};
			const b32 found = entity_player_find_nearest(epicenter,
														 human_player->team_side,
														 tmp_excluded_players_list,
														 &nearest_player,
														 game_state);

			if (found) {
				entity_id_t new_controlled_player_id = nearest_player.entity->header.id;

				if (new_controlled_player_id != EID_NOT_VALID &&
					new_controlled_player_id != human_player->controlled_player.entity_id) {
					// Hysteresis: only switch if the new player is meaningfully closer
					if (!force_switch && currently_selected_player_entity) {
						f32 current_distance = Vector2Distance(
							epicenter, currently_selected_player_entity->transform.position);
						if (nearest_player.distance > current_distance * 0.85f) {
							continue;
						}
					}
					// human is controlling a new player compared to previous frame
					switch_to_player(new_controlled_player_id, human_player, game_state);
				}
			}
		}
	}

	// for (usize i = 0; i < MAX_HUMAN_PLAYERS; ++i) {
	// 	struct human_player *human_player = &game_state->human_players[i];

	// 	if (human_player->enabled) {
	// 		printf("human player %d is controlling entity %d\n",
	// 			   human_player->id,
	// 			   human_player->controlled_player.entity_id);
	// 	}
	// }
}

static int compare_distances(const void *a, const void *b)
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

// Orders human players by distance from ball, nearest first.
// If human doesn't have a selected player it's scored last.
static struct list_human_players
order_humans_by_distance_from_ball(struct human_player *human_players,
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
													  human_player->controlled_player.entity_id);
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

static void switch_to_player(entity_id_t switch_to,
							 struct human_player *human_player,
							 struct game_state *game_state)
{
	input_reset(
		&game_state->input_manager->game_controllers[human_player->game_controller_id]);
	human_player->controlled_player.entity_id = switch_to;
}
