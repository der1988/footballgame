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
			igCheckbox("Show Radar", (bool *)&game_state->constants.show_radar);
			igCheckbox("Shoot Aid Auto Aim", (bool *)&game_state->constants.shoot_aid_auto_aim);
			igCheckbox("Shoot Aid No Inertia", (bool *)&game_state->constants.shoot_aid_no_inertia);
			igSliderFloat("Gravity force",
						  &game_state->constants.gravity_force.y,
						  -800.0f,
						  0.0f,
						  TextFormat("%.2f", game_state->constants.gravity_force.y),
						  0);
			igSliderFloat("Kick charge reset time",
						  &game_state->constants.kick_charge_reset_time,
						  0.0f,
						  2.0f,
						  TextFormat("%.2f", game_state->constants.kick_charge_reset_time),
						  0);
			igEndTabItem();
		}

		if (igBeginTabItem("Ball", NULL, 0)) {
			igSliderFloat("Coefficient of Restitution",
						  &game_state->constants.ball_cr,
						  0.0f,
						  1.0f,
						  TextFormat("%.2f", game_state->constants.ball_cr),
						  0);
			igSliderFloat("CR against post",
						  &game_state->constants.ball_cr_against_post,
						  0.0f,
						  1.0f,
						  TextFormat("%.2f", game_state->constants.ball_cr_against_post),
						  0);
			igSliderFloat("Drag coefficient",
						  &game_state->constants.ball_drag_coefficient,
						  0.0f,
						  1.0f,
						  TextFormat("%.2f", game_state->constants.ball_drag_coefficient),
						  0);
			igSliderInt("Friction on pitch",
						(int *)&game_state->constants.ball_rolling_resistance_mult,
						1,
						20,
						"%d",
						0);
			igSliderFloat("Max velocity",
						  &game_state->constants.ball_max_velocity,
						  0.0f,
						  1000.0f,
						  TextFormat("%.2f", game_state->constants.ball_max_velocity),
						  0);
			igSliderFloat("Max velocity to gain possession",
						  &game_state->constants.ball_max_velocity_to_gain_possession,
						  0.0f,
						  1000.0f,
						  TextFormat("%.2f",
									 game_state->constants.ball_max_velocity_to_gain_possession),
						  0);
			igSliderFloat("Run force",
						  &game_state->constants.ball_run_force,
						  0.0f,
						  200.0f,
						  TextFormat("%.2f", game_state->constants.ball_run_force),
						  0);
			igSliderFloat("Control force",
						  &game_state->constants.ball_control_force,
						  0.0f,
						  100.0f,
						  TextFormat("%.2f", game_state->constants.ball_control_force),
						  0);
			igSliderFloat("Shot force",
						  &game_state->constants.ball_shot_force,
						  0.0f,
						  1000.0f,
						  TextFormat("%.2f", game_state->constants.ball_shot_force),
						  0);
			igSliderFloat("Shot height",
						  &game_state->constants.ball_shot_height,
						  0.0f,
						  1.0f,
						  TextFormat("%.2f", game_state->constants.ball_shot_height),
						  0);
			igSliderFloat("Long pass force",
						  &game_state->constants.ball_long_pass_force,
						  0.0f,
						  1000.0f,
						  TextFormat("%.2f", game_state->constants.ball_long_pass_force),
						  0);
			igSliderFloat("Long pass height",
						  &game_state->constants.ball_long_pass_height,
						  0.0f,
						  1.0f,
						  TextFormat("%.2f", game_state->constants.ball_long_pass_height),
						  0);
			igSliderFloat("Short pass force",
						  &game_state->constants.ball_pass_force,
						  0.0f,
						  1000.0f,
						  TextFormat("%.2f", game_state->constants.ball_pass_force),
						  0);
			igSliderInt("Lookaheads",
						(int *)&game_state->constants.ball_lookaheads,
						1,
						BALL_MAX_LOOKAHEADS,
						"%d",
						0);
			igSliderInt(
				"Lookahead dts", (int *)&game_state->constants.ball_lookahead_dts, 10, 200, "%d", 0);
			igSliderFloat("After touch delay time",
						  &game_state->constants.ball_after_touch_delay_time,
						  0.0f,
						  1.0f,
						  TextFormat("%.2f", game_state->constants.ball_after_touch_delay_time),
						  0);
			igSliderFloat("After touch max time",
						  &game_state->constants.ball_after_touch_max_time,
						  0.0f,
						  1.0f,
						  TextFormat("%.2f", game_state->constants.ball_after_touch_max_time),
						  0);
			igSliderFloat("After touch force",
						  &game_state->constants.ball_after_touch_force,
						  0.0f,
						  1000.0f,
						  TextFormat("%.2f", game_state->constants.ball_after_touch_force),
						  0);
			igEndTabItem();
		}

		if (igBeginTabItem("Player", NULL, 0)) {
			igSliderFloat("Movement force",
						  &game_state->constants.player_movement_force,
						  0.0f,
						  100000.0f,
						  TextFormat("%.2f", game_state->constants.player_movement_force),
						  0);
			igSliderFloat("Movement drag",
						  &game_state->constants.player_movement_drag,
						  0.0f,
						  40000.0f,
						  TextFormat("%.2f", game_state->constants.player_movement_drag),
						  0);
			igSliderFloat("Control max velocity",
						  &game_state->constants.player_control_max_velocity,
						  0.0f,
						  500.0f,
						  TextFormat("%.2f", game_state->constants.player_control_max_velocity),
						  0);
			igSliderFloat("Run max velocity",
						  &game_state->constants.player_run_max_velocity,
						  0.0f,
						  500.0f,
						  TextFormat("%.2f", game_state->constants.player_run_max_velocity),
						  0);
			igSliderFloat("Control radius",
						  &game_state->constants.player_control_radius,
						  0.0f,
						  100.0f,
						  TextFormat("%.2f", game_state->constants.player_control_radius),
						  0);
			igSliderFloat("Control height",
						  &game_state->constants.player_control_height,
						  0.0f,
						  30.0f,
						  TextFormat("%.2f", game_state->constants.player_control_height),
						  0);
			igSliderFloat("Header min height",
						  &game_state->constants.player_header_min_height,
						  10.0f,
						  30.0f,
						  TextFormat("%.2f", game_state->constants.player_header_min_height),
						  0);
			igSliderFloat("Header max height",
						  &game_state->constants.player_header_max_height,
						  20.0f,
						  40.0f,
						  TextFormat("%.2f", game_state->constants.player_header_max_height),
						  0);
			igSliderFloat("Max reachable height",
						  &game_state->constants.player_max_reachable_height,
						  20.0f,
						  40.0f,
						  TextFormat("%.2f", game_state->constants.player_max_reachable_height),
						  0);
			igCheckbox("Pass target not selectable",
					   (bool *)&game_state->constants.player_pass_target_not_selectable);
			igEndTabItem();
		}

		if (igBeginTabItem("Goalie", NULL, 0)) {
			igSliderFloat("Reaction time",
						  &game_state->constants.goalie_dive_reaction_time,
						  0.0f,
						  0.4f,
						  TextFormat("%.2f", game_state->constants.goalie_dive_reaction_time),
						  0);
			igSliderFloat("Dive force",
						  &game_state->constants.goalie_dive_force,
						  5000.0f,
						  100000.0f,
						  TextFormat("%.2f", game_state->constants.goalie_dive_force),
						  0);
			igSliderFloat("Dive friction force",
						  &game_state->constants.goalie_dive_friction_force,
						  5000.0f,
						  100000.0f,
						  TextFormat("%.2f", game_state->constants.goalie_dive_friction_force),
						  0);
			igSliderFloat("Dive max velocity",
						  &game_state->constants.goalie_dive_max_velocity,
						  0.0f,
						  300.0f,
						  TextFormat("%.2f", game_state->constants.goalie_dive_max_velocity),
						  0);
			igSliderFloat("Dive min ball velocity",
						  &game_state->constants.goalie_dive_min_ball_velocity,
						  0.0f,
						  400.0f,
						  TextFormat("%.2f", game_state->constants.goalie_dive_min_ball_velocity),
						  0);
			igSliderFloat("Dive recover time",
						  &game_state->constants.goalie_dive_recover_time,
						  0.1f,
						  2.0f,
						  TextFormat("%.2f", game_state->constants.goalie_dive_recover_time),
						  0);
			igSliderFloat("Max reachable height",
						  &game_state->constants.goalie_max_reachable_height,
						  20.0f,
						  40.0f,
						  TextFormat("%.2f", game_state->constants.goalie_max_reachable_height),
						  0);
			igEndTabItem();
		}

		igEndTabBar();
	}

	igPopItemWidth();

	igEnd();
}
