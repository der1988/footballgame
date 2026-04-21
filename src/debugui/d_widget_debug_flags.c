#include "debugui/debugui.h"
#include "debugui/debugui_internal.h"
#include "game.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui/cimgui.h"
#include <assert.h>

void debugui_debug_flags_draw(debugui_t *handle, struct game_state *game_state)
{
	assert(handle->initialised);

	igSetNextWindowSize((ImVec2){handle->window_size.x, handle->window_size.y},
						ImGuiCond_FirstUseEver);

	igBegin("Debug Flags", NULL, 0);

	igPushItemWidth(350.0f);

	if (igBeginTabBar("TabBar", 0)) {
		if (igBeginTabItem("General", NULL, 0)) {
			igCheckbox("Debug Render", (bool *)&game_state->debug_flags.debug_render);
			igCheckbox("Show Bounding Boxes", (bool *)&game_state->debug_flags.show_bounding_boxes);
			igCheckbox("Show Pitch Lines", (bool *)&game_state->debug_flags.show_pitch_lines);
			igCheckbox("Show Cross Areas", (bool *)&game_state->debug_flags.show_cross_areas);
			igEndTabItem();
		}

		if (igBeginTabItem("Ball", NULL, 0)) {
			igCheckbox("Show Lookahead Positions",
					   (bool *)&game_state->debug_flags.show_ball_lookahead_positions);
			igCheckbox("Show Future Bounce Position",
					   (bool *)&game_state->debug_flags.show_ball_future_bounce_position);
			igEndTabItem();
		}

		if (igBeginTabItem("Colliders", NULL, 0)) {
			igCheckbox("Show Ball Collider", (bool *)&game_state->debug_flags.show_ball_collider);
			igCheckbox("Show Goal Collider", (bool *)&game_state->debug_flags.show_goal_collider);
			igCheckbox("Show Player Collider",
					   (bool *)&game_state->debug_flags.show_player_collider[PCT_PLAYER]);
			igCheckbox("Show Player Body Collider",
					   (bool *)&game_state->debug_flags.show_player_collider[PCT_BODY]);
			igCheckbox("Show Player Tackle 1 Collider",
					   (bool *)&game_state->debug_flags.show_player_collider[PCT_TACKLE_1]);
			igCheckbox("Show Player Tackle 2 Collider",
					   (bool *)&game_state->debug_flags.show_player_collider[PCT_TACKLE_2]);
			igCheckbox("Show Player Header Collider",
					   (bool *)&game_state->debug_flags.show_player_collider[PCT_HEADER]);
			igCheckbox("Show Player Goalie Collider",
					   (bool *)&game_state->debug_flags.show_player_collider[PCT_GOALIE]);
			igCheckbox("Show Player Dive Collider",
					   (bool *)&game_state->debug_flags.show_player_collider[PCT_DIVE]);
			igEndTabItem();
		}

		igEndTabBar();
	}

	igPopItemWidth();

	igEnd();
}
