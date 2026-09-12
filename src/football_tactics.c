#include "football.h"
#include "entity_components.h"
#include "entity.h"
#include "entity_ball.h"
#include "game.h"
#include <raylib.h>
#include <raymath.h>

struct team_formation_position team_formations[TF_COUNT][10] = {
	// TF_433
	{
		{
			.name = "LB",
			.style_of_play = SOP_BALANCED,
			.supported_roles = {PR_SB},
			.position =
				{
					{.x = 0.2f, .y = 0.1f},
					{.x = 0.1f, .y = 0.3f},
					{.x = 0.3f, .y = 0.65f},
				},
		},
		{
			.name = "LB",
			.style_of_play = SOP_BALANCED,
			.supported_roles = {PR_SB},
			.position =
				{
					{.x = 0.8f, .y = 0.1f},
					{.x = 0.9f, .y = 0.3f},
					{.x = 0.7f, .y = 0.65f},
				},
		},
		{
			.name = "CB",
			.style_of_play = SOP_DEFENSIVE,
			.supported_roles = {PR_CB},
			.position =
				{
					{.x = 0.4f, .y = 0.02f},
					{.x = 0.4f, .y = 0.2f},
					{.x = 0.4f, .y = 0.55f},
				},
		},
		{
			.name = "CB",
			.style_of_play = SOP_DEFENSIVE,
			.supported_roles = {PR_CB},
			.position =
				{
					{.x = 0.6f, .y = 0.02f},
					{.x = 0.6f, .y = 0.2f},
					{.x = 0.6f, .y = 0.55f},
				},
		},
		{
			.name = "DMF",
			.style_of_play = SOP_BALANCED,
			.supported_roles = {PR_DMF},
			.position =
				{
					{.x = 0.5f, .y = 0.15f},
					{.x = 0.5f, .y = 0.4f},
					{.x = 0.5f, .y = 0.75f},
				},
		},
		{
			.name = "CMF",
			.style_of_play = SOP_BALANCED,
			.supported_roles = {PR_CMF},
			.position =
				{
					{.x = 0.35f, .y = 0.25f},
					{.x = 0.35f, .y = 0.5f},
					{.x = 0.35f, .y = 0.85f},
				},
		},
		{
			.name = "CMF",
			.style_of_play = SOP_BALANCED,
			.supported_roles = {PR_CMF},
			.position = {{.x = 0.65f, .y = 0.25f},
						 {.x = 0.65f, .y = 0.5f},
						 {.x = 0.65f, .y = 0.85f}},
		},
		{
			.name = "LWF",
			.style_of_play = SOP_OFFENSIVE,
			.supported_roles = {PR_WF},
			.position =
				{
					{.x = 0.15f, .y = 0.35f},
					{.x = 0.15f, .y = 0.6f},
					{.x = 0.2f, .y = 0.92f},
				},
		},
		{
			.name = "RWF",
			.style_of_play = SOP_OFFENSIVE,
			.supported_roles = {PR_WF},
			.position =
				{
					{.x = 0.85f, .y = 0.35f},
					{.x = 0.85f, .y = 0.6f},
					{.x = 0.8f, .y = 0.92f},
				},
		},
		{
			.name = "CF",
			.style_of_play = SOP_BALANCED,
			.supported_roles = {PR_CF},
			.position =
				{
					{.x = 0.5f, .y = 0.45f},
					{.x = 0.5f, .y = 0.7f},
					{.x = 0.5f, .y = 0.95f},
				},
		},
	}};

struct team_formation_position *football_tactics_get_team_formation(enum team_formation formation)
{
	return team_formations[formation];
}

struct team_formation_position *
football_tactics_get_team_formation_position(enum team_formation formation, u32 position_idx)
{
	return &team_formations[formation][position_idx];
}

v2 football_tactics_get_target_position(struct entity *player_entity,
										struct entity *ball_entity,
										struct game_state *game_state)
{
	// A keeper has no slot in the formation, so he is placed against his own goal: up into his area
	// while his side has the ball, back on his line when it doesn't.
	if (entity_player_is_goalie(player_entity)) {
		const enum pitch_side goalie_pitch_side =
			game_state->vars.teams[player_entity->player.team_side].play_context.pitch_side;
		const struct goal *goal = &game_state->vars.pitch.goal[goalie_pitch_side];

		if (entity_ball_team_in_possession(ball_entity, game_state) ==
			player_entity->player.team_side) {
			// He follows the ball up and down: on his line while it is on top of his goal, out on
			// the edge of his area while his side has it at the other end.
			const v2 ball_normalized_position = football_pitch_position_abs_to_normalized(
				game_state->vars.pitch.field, ball_entity->transform.position);
			const f32 ball_distance_from_his_goal = goalie_pitch_side == PSI_NORTH ?
														ball_normalized_position.y :
														1.0f - ball_normalized_position.y;

			const v2 penalty_area_edge = {
				.x = goal->penalty_box.x + goal->penalty_box.width * 0.5f,
				.y = goalie_pitch_side == PSI_NORTH ?
						 goal->penalty_box.y + goal->penalty_box.height :
						 goal->penalty_box.y};

			return Vector2Lerp(
				goal->center, penalty_area_edge, Clamp(ball_distance_from_his_goal, 0, 1));
		}

		return goal->center;
	}

	struct team_formation_position *team_formation_position =
		football_tactics_get_team_formation_position(
			TF_433, player_entity->player.team_formation_position_idx);

	v2 ball_normalized_pos = football_pitch_position_abs_to_normalized(
		game_state->vars.pitch.field, ball_entity->transform.position);

	v2 default_pos = team_formation_position->position[TM_DEFAULT];
	v2 target_pos;

	enum pitch_side player_pitch_side =
		game_state->vars.teams[player_entity->player.team_side].play_context.pitch_side;

	switch (game_state->gameplay.state_machine.current_state) {
	case FS_PLAY: {
		if (ball_entity->ball.state_machine.current_state == BS_GK_HANDS) {
			target_pos = default_pos;
			break;
		}

		if (player_pitch_side == PSI_SOUTH) {
			if (ball_normalized_pos.y < 0.5f) {
				v2 attacking_pos = team_formation_position->position[TM_ATTACKING];
				target_pos = Vector2Lerp(attacking_pos, default_pos, ball_normalized_pos.y / 0.5f);
			} else {
				v2 defending_pos = team_formation_position->position[TM_DEFENDING];
				target_pos =
					Vector2Lerp(default_pos, defending_pos, (ball_normalized_pos.y - 0.5f) / 0.5f);
			}
		} else {
			if (ball_normalized_pos.y > 0.5f) {
				v2 attacking_pos = team_formation_position->position[TM_ATTACKING];
				target_pos =
					Vector2Lerp(default_pos, attacking_pos, (ball_normalized_pos.y - 0.5f) / 0.5f);
			} else {
				v2 defending_pos = team_formation_position->position[TM_DEFENDING];
				target_pos = Vector2Lerp(defending_pos, default_pos, ball_normalized_pos.y / 0.5f);
			}
		}

		// If ball is in the side lanes and player is on the other side of the pitch we need to
		// close the gap.
		if (ball_normalized_pos.x < 0.33f && target_pos.x > ball_normalized_pos.x) {
			f32 delta = target_pos.x - ball_normalized_pos.x;
			target_pos = Vector2Lerp(
				Vector2Add(target_pos, (v2){.x = -delta * (0.33f - ball_normalized_pos.x)}),
				target_pos,
				ball_normalized_pos.x / 0.33f);
		} else if (ball_normalized_pos.x > 0.66f && target_pos.x < ball_normalized_pos.x) {
			f32 delta = ball_normalized_pos.x - target_pos.x;
			target_pos = Vector2Lerp(target_pos,
									 Vector2Add(target_pos,
												(v2){.x = delta * (ball_normalized_pos.x - 0.66f)}),
									 (ball_normalized_pos.x - 0.66f) / 0.33f);
		}
	} break;
	case FS_NONE:
	case FS_KICKOFF:
	case FS_HALF_TIME:
	case FS_EXTRA_TIME:
	case FS_PENALTIES:
	case FS_GOAL:
	case FS_THROW_IN:
	case FS_GOAL_KICK:
	case FS_CORNER_KICK:
	case FS_PENALTY_KICK:
	case FS_FREE_KICK:
	case FS_COUNT:
		target_pos = default_pos;
		break;
	}

	if (player_pitch_side == PSI_NORTH) {
		target_pos.y = 1 - target_pos.y;
	}

	return football_pitch_position_normalized_to_abs(game_state->vars.pitch.field, target_pos);
}
