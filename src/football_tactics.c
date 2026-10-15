#include "football.h"
#include "entity_components.h"
#include "entity.h"
#include "entity_ball.h"
#include "game.h"
#include <raylib.h>
#include <raymath.h>
#include <string.h>

#define SLOT(N, R, S, X, D, B, A, AX) { .name=N, .supported_roles={R}, .style_of_play=S, .position={{X,D},{X,B},{AX,A}} }

struct team_formation_position team_formations[TF_COUNT][10] = {
	// TF_433
	[TF_433] = {
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
	},
	[TF_541] = {
		SLOT("LWB", PR_WB, SOP_BALANCED, 0.1f, 0.13f, 0.26f, 0.51f, 0.1f),
		SLOT("LCB", PR_CB, SOP_DEFENSIVE, 0.32f, 0.08f, 0.2f, 0.4f, 0.32f),
		SLOT("CB", PR_CB, SOP_DEFENSIVE, 0.5f, 0.07f, 0.19f, 0.39f, 0.5f),
		SLOT("RCB", PR_CB, SOP_DEFENSIVE, 0.68f, 0.08f, 0.2f, 0.4f, 0.68f),
		SLOT("RWB", PR_WB, SOP_BALANCED, 0.9f, 0.13f, 0.26f, 0.51f, 0.9f),
		SLOT("LM", PR_SMF, SOP_BALANCED, 0.18f, 0.31f, 0.43f, 0.67f, 0.13f),
		SLOT("LCM", PR_CMF, SOP_BALANCED, 0.4f, 0.29f, 0.42f, 0.62f, 0.4f),
		SLOT("RCM", PR_CMF, SOP_BALANCED, 0.6f, 0.29f, 0.42f, 0.62f, 0.6f),
		SLOT("RM", PR_SMF, SOP_BALANCED, 0.82f, 0.31f, 0.43f, 0.67f, 0.87f),
		SLOT("CF", PR_CF, SOP_OFFENSIVE, 0.5f, 0.58f, 0.68f, 0.87f, 0.5f),
	},
	[TF_532] = {
		SLOT("LWB", PR_WB, SOP_BALANCED, 0.1f, 0.14f, 0.29f, 0.7f, 0.1f),
		SLOT("LCB", PR_CB, SOP_DEFENSIVE, 0.32f, 0.1f, 0.22f, 0.45f, 0.32f),
		SLOT("CB", PR_CB, SOP_DEFENSIVE, 0.5f, 0.09f, 0.21f, 0.43f, 0.5f),
		SLOT("RCB", PR_CB, SOP_DEFENSIVE, 0.68f, 0.1f, 0.22f, 0.45f, 0.68f),
		SLOT("RWB", PR_WB, SOP_BALANCED, 0.9f, 0.14f, 0.29f, 0.7f, 0.9f),
		SLOT("LCM", PR_CMF, SOP_BALANCED, 0.3f, 0.32f, 0.45f, 0.68f, 0.3f),
		SLOT("DM", PR_DMF, SOP_BALANCED, 0.5f, 0.27f, 0.4f, 0.59f, 0.5f),
		SLOT("RCM", PR_CMF, SOP_BALANCED, 0.7f, 0.32f, 0.45f, 0.68f, 0.7f),
		SLOT("LCF", PR_CF, SOP_OFFENSIVE, 0.4f, 0.58f, 0.69f, 0.91f, 0.37f),
		SLOT("RCF", PR_CF, SOP_OFFENSIVE, 0.6f, 0.58f, 0.69f, 0.91f, 0.63f),
	},
	[TF_442] = {
		SLOT("LB", PR_SB, SOP_BALANCED, 0.14f, 0.15f, 0.26f, 0.63f, 0.1f),
		SLOT("LCB", PR_CB, SOP_DEFENSIVE, 0.39f, 0.11f, 0.23f, 0.47f, 0.39f),
		SLOT("RCB", PR_CB, SOP_DEFENSIVE, 0.61f, 0.11f, 0.23f, 0.47f, 0.61f),
		SLOT("RB", PR_SB, SOP_BALANCED, 0.86f, 0.15f, 0.26f, 0.63f, 0.9f),
		SLOT("LM", PR_SMF, SOP_BALANCED, 0.16f, 0.35f, 0.49f, 0.76f, 0.13f),
		SLOT("LCM", PR_CMF, SOP_BALANCED, 0.4f, 0.32f, 0.46f, 0.7f, 0.4f),
		SLOT("RCM", PR_CMF, SOP_BALANCED, 0.6f, 0.32f, 0.46f, 0.7f, 0.6f),
		SLOT("RM", PR_SMF, SOP_BALANCED, 0.84f, 0.35f, 0.49f, 0.76f, 0.87f),
		SLOT("LCF", PR_CF, SOP_OFFENSIVE, 0.4f, 0.57f, 0.7f, 0.91f, 0.37f),
		SLOT("RCF", PR_CF, SOP_OFFENSIVE, 0.6f, 0.57f, 0.7f, 0.91f, 0.63f),
	},
	[TF_343] = {
		SLOT("LCB", PR_CB, SOP_DEFENSIVE, 0.28f, 0.2f, 0.33f, 0.57f, 0.25f),
		SLOT("CB", PR_CB, SOP_DEFENSIVE, 0.5f, 0.18f, 0.31f, 0.54f, 0.5f),
		SLOT("RCB", PR_CB, SOP_DEFENSIVE, 0.72f, 0.2f, 0.33f, 0.57f, 0.75f),
		SLOT("LWB", PR_WB, SOP_BALANCED, 0.1f, 0.33f, 0.53f, 0.86f, 0.08f),
		SLOT("LCM", PR_CMF, SOP_BALANCED, 0.4f, 0.38f, 0.53f, 0.76f, 0.38f),
		SLOT("RCM", PR_CMF, SOP_BALANCED, 0.6f, 0.38f, 0.53f, 0.76f, 0.62f),
		SLOT("RWB", PR_WB, SOP_BALANCED, 0.9f, 0.33f, 0.53f, 0.86f, 0.92f),
		SLOT("LWF", PR_WF, SOP_OFFENSIVE, 0.2f, 0.53f, 0.73f, 0.94f, 0.16f),
		SLOT("CF", PR_CF, SOP_OFFENSIVE, 0.5f, 0.62f, 0.8f, 0.97f, 0.5f),
		SLOT("RWF", PR_WF, SOP_OFFENSIVE, 0.8f, 0.53f, 0.73f, 0.94f, 0.84f),
	},
	[TF_3232] = {
		SLOT("LCB", PR_CB, SOP_DEFENSIVE, 0.28f, 0.19f, 0.33f, 0.57f, 0.25f),
		SLOT("CB", PR_CB, SOP_DEFENSIVE, 0.5f, 0.17f, 0.31f, 0.53f, 0.5f),
		SLOT("RCB", PR_CB, SOP_DEFENSIVE, 0.72f, 0.19f, 0.33f, 0.57f, 0.75f),
		SLOT("LDM", PR_DMF, SOP_BALANCED, 0.4f, 0.34f, 0.48f, 0.69f, 0.38f),
		SLOT("RDM", PR_DMF, SOP_BALANCED, 0.6f, 0.34f, 0.48f, 0.69f, 0.62f),
		SLOT("LAM", PR_AMF, SOP_OFFENSIVE, 0.16f, 0.48f, 0.67f, 0.92f, 0.12f),
		SLOT("CAM", PR_AMF, SOP_OFFENSIVE, 0.5f, 0.48f, 0.67f, 0.88f, 0.5f),
		SLOT("RAM", PR_AMF, SOP_OFFENSIVE, 0.84f, 0.48f, 0.67f, 0.92f, 0.88f),
		SLOT("LCF", PR_CF, SOP_OFFENSIVE, 0.4f, 0.62f, 0.81f, 0.97f, 0.37f),
		SLOT("RCF", PR_CF, SOP_OFFENSIVE, 0.6f, 0.62f, 0.81f, 0.97f, 0.63f),
	},
};
#undef SLOT

static enum team_formation supported_formation(enum team_formation formation)
{
	return formation >= 0 && formation < TF_COUNT && team_formations[formation][0].name ? formation : TF_433;
}

const struct team_tactics_profile *football_tactics_get_profile(enum team_formation formation)
{
	static const struct team_tactics_profile defensive = {125, 0.78f, 0.9f};
	static const struct team_tactics_profile counter = {170, 0.85f, 1.0f};
	static const struct team_tactics_profile balanced = {240, 0.94f, 1.0f};
	static const struct team_tactics_profile attacking = {340, 1.0f, 1.12f};
	static const struct team_tactics_profile overload = {360, 0.96f, 1.03f};
	switch (formation) {
	case TF_541: return &defensive;
	case TF_532: return &counter;
	case TF_343: return &attacking;
	case TF_3232: return &overload;
	default: return &balanced;
	}
}

enum team_formation football_tactics_formation_from_code(const char *code)
{
	if (!code) return TF_442;
	if (strcmp(code, "541") == 0) return TF_541;
	if (strcmp(code, "532") == 0) return TF_532;
	if (strcmp(code, "343") == 0) return TF_343;
	if (strcmp(code, "3232") == 0) return TF_3232;
	if (strcmp(code, "433") == 0) return TF_433;
	return TF_442;
}

struct team_formation_position *football_tactics_get_team_formation(enum team_formation formation)
{
	return team_formations[supported_formation(formation)];
}

struct team_formation_position *
football_tactics_get_team_formation_position(enum team_formation formation, u32 position_idx)
{
	ASSERT(position_idx < 10);
	return &team_formations[supported_formation(formation)][position_idx];
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

	const enum team_side side = player_entity->player.team_side;
	const struct team_play_context *team = &game_state->vars.teams[side].play_context;
	const struct team_formation_position *slot = football_tactics_get_team_formation_position(
		team->formation, player_entity->player.team_formation_position_idx);
	const struct team_tactics_profile *profile = football_tactics_get_profile(team->formation);
	v2 ball = football_pitch_position_abs_to_normalized(game_state->vars.pitch.field, ball_entity->transform.position);
	// All stored shapes run from our goal (0) to the opposing goal (1).
	const f32 progress = Clamp(team->pitch_side == PSI_NORTH ? ball.y : 1 - ball.y, 0, 1);
	v2 target = slot->position[TM_DEFAULT];
	if (game_state->gameplay.state_machine.current_state == FS_PLAY &&
		ball_entity->ball.state_machine.current_state != BS_GK_HANDS) {
		const b32 in_possession = entity_ball_team_in_possession(ball_entity, game_state) == side;
		if (in_possession) {
			target = Vector2Lerp(target, slot->position[TM_ATTACKING], 0.35f + 0.65f * progress);
			target.x = 0.5f + (target.x - 0.5f) * profile->attacking_width;
		} else {
			target = Vector2Lerp(slot->position[TM_DEFENDING], target, 0.25f * progress);
			target.x = 0.5f + (target.x - 0.5f) * profile->defensive_width;
		}
		// Slide across together to close the ball-side lane, without abandoning the far side.
		target.x += (ball.x - 0.5f)*(in_possession ? 0.10f : 0.20f);
	}
	target.x = Clamp(target.x, 0.05f, 0.95f);
	target.y = Clamp(target.y, 0.04f, 0.97f);
	// The pitch conversion below consumes bottom-up coordinates.
	if (team->pitch_side == PSI_NORTH) target.y = 1 - target.y;
	return football_pitch_position_normalized_to_abs(game_state->vars.pitch.field, target);
}
