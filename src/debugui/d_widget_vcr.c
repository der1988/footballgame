#include "game.h"
#include "time_machine.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui/cimgui.h"
#include "debugui_internal.h"
#include "debugui.h"
#include <assert.h>

void debugui_vcr_draw(debugui_t *handle, struct game_state *game_state)
{
	assert(handle->initialised);

	igSetNextWindowSize((ImVec2){handle->window_size.x, handle->window_size.y},
						ImGuiCond_FirstUseEver);

	igBegin("VCR", NULL, 0);

	struct time_machine *tm = game_state->time_machine;
	if (tm->recorded == 0) {
		int v = 0;
		igBeginDisabled(true);
		igSliderInt("##frame", &v, 0, 0, "0 / 0", 0);
		igEndDisabled();
	} else {
		int max_frame = (int)(tm->recorded - 1);
		int cur_frame = (int)(time_machine_current_pos(tm));
		int prev_frame = cur_frame;

		igPushItemWidth(-1);
		if (igSliderInt("##frame",
						&cur_frame,
						0,
						max_frame,
						TextFormat("%d / %d", cur_frame, max_frame),
						0)) {
			time_machine_travel(tm, cur_frame - prev_frame, game_state);
		}
		igPopItemWidth();
	}

	if (igIsWindowHovered(0)) {
		f32 wheel = GetMouseWheelMove();
		if (wheel != 0) {
			time_machine_travel(tm, (size)wheel, game_state);
		}
	}

	if (igButton("Resume", (ImVec2){100, 40})) {
		time_machine_set_resume_position(tm, RP_CURRENT, game_state);
		game_state->vars.mode = GM_TICK;
	}

	igEnd();
}
