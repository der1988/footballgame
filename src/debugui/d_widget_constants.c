#include "debugui/debugui.h"
#include "debugui/debugui_internal.h"
#include "game.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui/cimgui.h"
#include <assert.h>

void debugui_constants_inspector_draw(debugui_t *handle, struct game_state *game_state)
{
	assert(handle->initialised);

	igShowDemoWindow(NULL);

	igSetNextWindowSize((ImVec2){handle->window_size.x, handle->window_size.y},
						ImGuiCond_FirstUseEver);

	igBegin("Constants Inspector", NULL, 0);

	igPushItemWidth(350.0f);

	if (igBeginTabBar("TabBar", 0)) {
		if (igBeginTabItem("General", NULL, 0)) {
			igCheckbox("AI Enabled", (bool *)&game_state->constants.ai_enabled);
			igCheckbox("Shoot Aid No Inertia", (bool *)&game_state->constants.shoot_aid_no_inertia);
			igSliderFloat("Gravity force",
						  &game_state->constants.gravity_force.y,
						  -800.0f,
						  0.0f,
						  TextFormat("%.2f", game_state->constants.gravity_force.y),
						  0);
			igEndTabItem();
		}

		if (igBeginTabItem("Ball", NULL, 0)) {
			igSliderFloat("Ball CR",
						  &game_state->constants.ball_cr,
						  0.0f,
						  1.0f,
						  TextFormat("%.2f", game_state->constants.ball_cr),
						  0);
			igSliderFloat("Ball CR against post",
						  &game_state->constants.ball_cr_against_post,
						  0.0f,
						  1.0f,
						  TextFormat("%.2f", game_state->constants.ball_cr_against_post),
						  0);
			igSliderFloat("Ball drag coefficient",
						  &game_state->constants.ball_drag_coefficient,
						  0.0f,
						  1.0f,
						  TextFormat("%.2f", game_state->constants.ball_drag_coefficient),
						  0);
			igSliderInt("Ball friction on pitch",
						(int *)&game_state->constants.ball_rolling_resistance_mult,
						1,
						20,
						"%d",
						0);
			igSliderFloat("Ball run force",
						  &game_state->constants.ball_run_force,
						  0.0f,
						  200.0f,
						  TextFormat("%.2f", game_state->constants.ball_run_force),
						  0);
			igSliderFloat("Ball control force",
						  &game_state->constants.ball_control_force,
						  0.0f,
						  100.0f,
						  TextFormat("%.2f", game_state->constants.ball_control_force),
						  0);
			igSliderFloat("Ball shot force",
						  &game_state->constants.ball_shot_force,
						  0.0f,
						  1000.0f,
						  TextFormat("%.2f", game_state->constants.ball_shot_force),
						  0);
			igSliderFloat("Ball shot height",
						  &game_state->constants.ball_shot_height,
						  0.0f,
						  1.0f,
						  TextFormat("%.2f", game_state->constants.ball_shot_height),
						  0);
			igSliderFloat("Ball long pass force",
						  &game_state->constants.ball_long_pass_force,
						  0.0f,
						  1000.0f,
						  TextFormat("%.2f", game_state->constants.ball_long_pass_force),
						  0);
			igSliderFloat("Ball long pass height",
						  &game_state->constants.ball_long_pass_height,
						  0.0f,
						  1.0f,
						  TextFormat("%.2f", game_state->constants.ball_long_pass_height),
						  0);
			igSliderFloat("Ball short pass force",
						  &game_state->constants.ball_pass_force,
						  0.0f,
						  1000.0f,
						  TextFormat("%.2f", game_state->constants.ball_pass_force),
						  0);
			igSliderInt(
				"Ball lookaheads", (int *)&game_state->constants.ball_lookaheads, 1, 10, "%d", 0);
			igSliderInt("Ball lookahead dts",
						(int *)&game_state->constants.ball_lookahead_dts,
						10,
						200,
						"%d",
						0);
			igEndTabItem();
		}

		if (igBeginTabItem("Player", NULL, 0)) {
			igCheckbox("Player pass target not selectable",
					   (bool *)&game_state->constants.player_pass_target_not_selectable);
			igSliderFloat("Player movement force",
						  &game_state->constants.player_movement_force,
						  0.0f,
						  40000.0f,
						  TextFormat("%.2f", game_state->constants.player_movement_force),
						  0);
			igSliderFloat("Player movement drag",
						  &game_state->constants.player_movement_drag,
						  0.0f,
						  40000.0f,
						  TextFormat("%.2f", game_state->constants.player_movement_drag),
						  0);
			igSliderFloat("Player control max velocity",
						  &game_state->constants.player_control_max_velocity,
						  0.0f,
						  500.0f,
						  TextFormat("%.2f", game_state->constants.player_control_max_velocity),
						  0);
			igSliderFloat("Player run max velocity",
						  &game_state->constants.player_run_max_velocity,
						  0.0f,
						  500.0f,
						  TextFormat("%.2f", game_state->constants.player_run_max_velocity),
						  0);
			igSliderFloat("Player control radius",
						  &game_state->constants.player_control_radius,
						  0.0f,
						  100.0f,
						  TextFormat("%.2f", game_state->constants.player_control_radius),
						  0);
			igSliderFloat("Player control height",
						  &game_state->constants.player_control_height,
						  0.0f,
						  30.0f,
						  TextFormat("%.2f", game_state->constants.player_control_height),
						  0);
			igSliderFloat("Player control run max angle",
						  &game_state->constants.player_run_control_max_angle,
						  0.0f,
						  60.0f,
						  TextFormat("%.2f", game_state->constants.player_run_control_max_angle),
						  0);
			igEndTabItem();
		}

		if (igBeginTabItem("Goalie", NULL, 0)) {
			igSliderFloat("Goalie reaction time",
						  &game_state->constants.goalie_dive_reaction_time,
						  0.0f,
						  0.4f,
						  TextFormat("%.2f", game_state->constants.goalie_dive_reaction_time),
						  0);
			igSliderFloat("Goalie dive force",
						  &game_state->constants.goalie_dive_force,
						  5000.0f,
						  20000.0f,
						  TextFormat("%.2f", game_state->constants.goalie_dive_force),
						  0);
			igSliderFloat("Goalie dive max velocity",
						  &game_state->constants.goalie_dive_max_velocity,
						  0.0f,
						  300.0f,
						  TextFormat("%.2f", game_state->constants.goalie_dive_max_velocity),
						  0);
			igSliderFloat("Goalie dive min ball velocity",
						  &game_state->constants.goalie_dive_min_ball_velocity,
						  0.0f,
						  400.0f,
						  TextFormat("%.2f", game_state->constants.goalie_dive_min_ball_velocity),
						  0);
			igSliderFloat("Kick charge reset time",
						  &game_state->constants.kick_charge_reset_time,
						  0.0f,
						  2.0f,
						  TextFormat("%.2f", game_state->constants.kick_charge_reset_time),
						  0);
			igEndTabItem();
		}

		igEndTabBar();
	}

	igPopItemWidth();

	igEnd();
}
