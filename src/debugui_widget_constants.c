#include "macro.h"
#include "debugui.h"
#include "debugui_internal.h"
#include "game.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui/cimgui.h>

void debugui_constants_inspector_draw(debugui_t *handle, struct game_state *game_state)
{
	ASSERT(handle->initialised);

	igShowDemoWindow(NULL);

	igSetNextWindowSize((ImVec2){handle->window_size.x, handle->window_size.y},
						ImGuiCond_FirstUseEver);

	igBegin("Constants Inspector", NULL, 0);

	igPushItemWidth(350.0f);

	// Accumulate whether any constant was edited this frame. We don't act on it here: the dirty
	// flag is consumed (and EVT_CONSTANTS_CHANGED published once) when the debug UI is dismissed,
	// so we recompute a single time per editing session instead of every drag frame.
	bool changed = false;

	if (igBeginTabBar("TabBar", 0)) {
		if (igBeginTabItem("General", NULL, 0)) {
			changed |= igCheckbox("AI Enabled", (bool *)&game_state->constants.ai_enabled);
			changed |= igCheckbox("Show Radar", (bool *)&game_state->constants.show_radar);
			changed |=
				igCheckbox("Shoot Aid Auto Aim", (bool *)&game_state->constants.shoot_aid_auto_aim);
			changed |= igCheckbox("Shoot Aid No Inertia",
								  (bool *)&game_state->constants.shoot_aid_no_inertia);
			changed |= igSliderFloat("Gravity force",
									 &game_state->constants.gravity_force.y,
									 -800.0f,
									 0.0f,
									 TextFormat("%.2f", game_state->constants.gravity_force.y),
									 0);
			changed |=
				igSliderFloat("Kick charge reset time",
							  &game_state->constants.kick_charge_reset_time,
							  0.0f,
							  2.0f,
							  TextFormat("%.2f", game_state->constants.kick_charge_reset_time),
							  0);
			igEndTabItem();
		}

		if (igBeginTabItem("Ball", NULL, 0)) {
			changed |= igSliderFloat("Coefficient of Restitution",
									 &game_state->constants.ball_cr,
									 0.0f,
									 1.0f,
									 TextFormat("%.2f", game_state->constants.ball_cr),
									 0);
			changed |= igSliderFloat("CR against post",
									 &game_state->constants.ball_cr_against_post,
									 0.0f,
									 1.0f,
									 TextFormat("%.2f", game_state->constants.ball_cr_against_post),
									 0);
			changed |=
				igSliderFloat("Drag coefficient",
							  &game_state->constants.ball_drag_coefficient,
							  0.0f,
							  1.0f,
							  TextFormat("%.2f", game_state->constants.ball_drag_coefficient),
							  0);
			changed |= igSliderInt("Friction on pitch",
								   (int *)&game_state->constants.ball_rolling_resistance_mult,
								   1,
								   20,
								   "%d",
								   0);
			changed |= igSliderFloat("Max velocity",
									 &game_state->constants.ball_max_velocity,
									 0.0f,
									 1000.0f,
									 TextFormat("%.2f", game_state->constants.ball_max_velocity),
									 0);
			changed |= igSliderFloat(
				"Max velocity to gain possession",
				&game_state->constants.ball_max_velocity_to_gain_possession,
				0.0f,
				1000.0f,
				TextFormat("%.2f", game_state->constants.ball_max_velocity_to_gain_possession),
				0);
			changed |= igSliderFloat("Run force",
									 &game_state->constants.ball_run_force,
									 0.0f,
									 200.0f,
									 TextFormat("%.2f", game_state->constants.ball_run_force),
									 0);
			changed |= igSliderFloat("Control force",
									 &game_state->constants.ball_control_force,
									 0.0f,
									 100.0f,
									 TextFormat("%.2f", game_state->constants.ball_control_force),
									 0);
			changed |= igSliderFloat("Shot force",
									 &game_state->constants.ball_shot_force,
									 0.0f,
									 1000.0f,
									 TextFormat("%.2f", game_state->constants.ball_shot_force),
									 0);
			changed |= igSliderFloat("Shot height",
									 &game_state->constants.ball_shot_height,
									 0.0f,
									 1.0f,
									 TextFormat("%.2f", game_state->constants.ball_shot_height),
									 0);
			changed |= igSliderFloat("Chip shot force",
									 &game_state->constants.ball_chip_shot_force,
									 0.0f,
									 1000.0f,
									 TextFormat("%.2f", game_state->constants.ball_chip_shot_force),
									 0);
			changed |=
				igSliderFloat("Chip shot height",
							  &game_state->constants.ball_chip_shot_height,
							  0.0f,
							  1.0f,
							  TextFormat("%.2f", game_state->constants.ball_chip_shot_height),
							  0);
			changed |= igSliderFloat("Long pass force",
									 &game_state->constants.ball_long_pass_force,
									 0.0f,
									 1000.0f,
									 TextFormat("%.2f", game_state->constants.ball_long_pass_force),
									 0);
			changed |=
				igSliderFloat("Long pass height",
							  &game_state->constants.ball_long_pass_height,
							  0.0f,
							  1.0f,
							  TextFormat("%.2f", game_state->constants.ball_long_pass_height),
							  0);
			changed |=
				igSliderFloat("Long driven pass force",
							  &game_state->constants.ball_long_driven_pass_force,
							  0.0f,
							  1000.0f,
							  TextFormat("%.2f", game_state->constants.ball_long_driven_pass_force),
							  0);
			changed |= igSliderFloat("Long driven pass height",
									 &game_state->constants.ball_long_driven_pass_height,
									 0.0f,
									 1.0f,
									 TextFormat("%.2f",
												game_state->constants.ball_long_driven_pass_height),
									 0);
			changed |= igSliderFloat("Tackle force",
									 &game_state->constants.ball_tackle_force,
									 0.0f,
									 1000.0f,
									 TextFormat("%.2f", game_state->constants.ball_tackle_force),
									 0);
			changed |= igSliderFloat(
				"Sliding tackle force",
				&game_state->constants.ball_sliding_tackle_force,
				0.0f,
				1000.0f,
				TextFormat("%.2f", game_state->constants.ball_sliding_tackle_force),
				0);
			changed |= igSliderFloat("Short pass force",
									 &game_state->constants.ball_pass_force,
									 0.0f,
									 1000.0f,
									 TextFormat("%.2f", game_state->constants.ball_pass_force),
									 0);
			changed |=
				igSliderFloat("Auto pass max distance",
							  &game_state->constants.ball_pass_auto_max_distance,
							  50.0f,
							  600.0f,
							  TextFormat("%.0f", game_state->constants.ball_pass_auto_max_distance),
							  0);
			changed |=
				igSliderFloat("Auto pass lead amount",
							  &game_state->constants.ball_pass_auto_lead_amount,
							  0.0f,
							  1.5f,
							  TextFormat("%.2f", game_state->constants.ball_pass_auto_lead_amount),
							  0);
			changed |=
				igSliderFloat("Header min force",
							  &game_state->constants.ball_header_min_force,
							  0.0f,
							  500.0f,
							  TextFormat("%.2f", game_state->constants.ball_header_min_force),
							  0);
			changed |=
				igSliderFloat("Header fumble force",
							  &game_state->constants.ball_header_fumble_force,
							  0.0f,
							  200.0f,
							  TextFormat("%.2f", game_state->constants.ball_header_fumble_force),
							  0);
			changed |= igSliderInt("Lookaheads",
								   (int *)&game_state->constants.ball_lookaheads,
								   1,
								   BALL_MAX_LOOKAHEADS,
								   "%d",
								   0);
			changed |= igSliderInt(
				"Lookahead dts", (int *)&game_state->constants.ball_lookahead_dts, 10, 200, "%d", 0);
			changed |=
				igSliderFloat("After touch delay time",
							  &game_state->constants.ball_after_touch_delay_time,
							  0.0f,
							  1.0f,
							  TextFormat("%.2f", game_state->constants.ball_after_touch_delay_time),
							  0);
			changed |=
				igSliderFloat("After touch max time",
							  &game_state->constants.ball_after_touch_max_time,
							  0.0f,
							  1.0f,
							  TextFormat("%.2f", game_state->constants.ball_after_touch_max_time),
							  0);
			changed |=
				igSliderFloat("After touch force",
							  &game_state->constants.ball_after_touch_force,
							  0.0f,
							  1000.0f,
							  TextFormat("%.2f", game_state->constants.ball_after_touch_force),
							  0);
			igEndTabItem();
		}

		if (igBeginTabItem("Player", NULL, 0)) {
			changed |=
				igSliderFloat("Movement force",
							  &game_state->constants.player_movement_force,
							  0.0f,
							  100000.0f,
							  TextFormat("%.2f", game_state->constants.player_movement_force),
							  0);
			changed |= igSliderFloat("Movement drag",
									 &game_state->constants.player_movement_drag,
									 0.0f,
									 40000.0f,
									 TextFormat("%.2f", game_state->constants.player_movement_drag),
									 0);
			changed |=
				igSliderFloat("Control max velocity",
							  &game_state->constants.player_control_max_velocity,
							  0.0f,
							  500.0f,
							  TextFormat("%.2f", game_state->constants.player_control_max_velocity),
							  0);
			changed |=
				igSliderFloat("Run max velocity",
							  &game_state->constants.player_run_max_velocity,
							  0.0f,
							  500.0f,
							  TextFormat("%.2f", game_state->constants.player_run_max_velocity),
							  0);
			changed |=
				igSliderFloat("Control radius",
							  &game_state->constants.player_control_radius,
							  0.0f,
							  100.0f,
							  TextFormat("%.2f", game_state->constants.player_control_radius),
							  0);
			changed |=
				igSliderFloat("Control height",
							  &game_state->constants.player_control_height,
							  0.0f,
							  30.0f,
							  TextFormat("%.2f", game_state->constants.player_control_height),
							  0);
			changed |= igSliderFloat("Tackle friction force",
									 &game_state->constants.player_tackle_friction_force,
									 0.0f,
									 100000.0f,
									 TextFormat("%.2f",
												game_state->constants.player_tackle_friction_force),
									 0);
			changed |= igSliderFloat("Sliding tackle friction force",
									 &game_state->constants.player_sliding_tackle_friction_force,
									 0.0f,
									 100000.0f,
									 TextFormat("%.2f",
												game_state->constants.player_sliding_tackle_friction_force),
									 0);
			changed |=
				igSliderFloat("Dribble min velocity",
							  &game_state->constants.player_dribble_min_velocity,
							  0.0f,
							  200.0f,
							  TextFormat("%.2f", game_state->constants.player_dribble_min_velocity),
							  0);
			changed |= igSliderFloat("Dribble max alignment",
									 &game_state->constants.player_dribble_max_alignment,
									 -1.0f,
									 1.0f,
									 TextFormat("%.2f",
												game_state->constants.player_dribble_max_alignment),
									 0);
			changed |=
				igSliderFloat("Dribble min time",
							  &game_state->constants.player_dribble_min_time,
							  0.0f,
							  0.5f,
							  TextFormat("%.2f", game_state->constants.player_dribble_min_time),
							  0);
			changed |=
				igSliderFloat("Dribble max time",
							  &game_state->constants.player_dribble_max_time,
							  0.0f,
							  1.0f,
							  TextFormat("%.2f", game_state->constants.player_dribble_max_time),
							  0);
			changed |=
				igSliderFloat("Tackled run time",
							  &game_state->constants.player_tackled_run_time,
							  0.0f,
							  3.0f,
							  TextFormat("%.2f", game_state->constants.player_tackled_run_time),
							  0);
			changed |= igSliderFloat(
				"Tackled run min velocity",
				&game_state->constants.player_tackled_run_min_velocity,
				0.0f,
				120.0f,
				TextFormat("%.2f", game_state->constants.player_tackled_run_min_velocity),
				0);
			changed |=
				igSliderFloat("Goal side bonus",
							  &game_state->constants.player_goal_side_bonus,
							  0.0f,
							  1.0f,
							  TextFormat("%.2f", game_state->constants.player_goal_side_bonus),
							  0);
			changed |=
				igSliderFloat("Switch grace time",
							  &game_state->constants.player_switch_grace_time,
							  0.0f,
							  2.0f,
							  TextFormat("%.2f", game_state->constants.player_switch_grace_time),
							  0);
			igEndTabItem();
		}

		if (igBeginTabItem("Goalie", NULL, 0)) {
			changed |=
				igSliderFloat("Reaction time",
							  &game_state->constants.goalie_dive_reaction_time,
							  0.0f,
							  0.4f,
							  TextFormat("%.2f", game_state->constants.goalie_dive_reaction_time),
							  0);
			changed |= igSliderFloat("Dive force",
									 &game_state->constants.goalie_dive_force,
									 5000.0f,
									 100000.0f,
									 TextFormat("%.2f", game_state->constants.goalie_dive_force),
									 0);
			changed |=
				igSliderFloat("Dive friction force",
							  &game_state->constants.goalie_dive_friction_force,
							  5000.0f,
							  100000.0f,
							  TextFormat("%.2f", game_state->constants.goalie_dive_friction_force),
							  0);
			changed |=
				igSliderFloat("Dive max velocity",
							  &game_state->constants.goalie_dive_max_velocity,
							  0.0f,
							  300.0f,
							  TextFormat("%.2f", game_state->constants.goalie_dive_max_velocity),
							  0);
			changed |= igSliderFloat(
				"Dive min ball velocity",
				&game_state->constants.goalie_dive_min_ball_velocity,
				0.0f,
				400.0f,
				TextFormat("%.2f", game_state->constants.goalie_dive_min_ball_velocity),
				0);
			changed |=
				igSliderFloat("Dive recover time",
							  &game_state->constants.goalie_dive_recover_time,
							  0.1f,
							  2.0f,
							  TextFormat("%.2f", game_state->constants.goalie_dive_recover_time),
							  0);
			changed |=
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

	if (changed) {
		handle->constants_dirty = true;
	}

	igPopItemWidth();

	igEnd();
}
