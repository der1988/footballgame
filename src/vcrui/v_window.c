#include "game.h"
#include "time_machine.h"
#include "rlimgui/rlImGui.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui/cimgui.h"
#include "vcrui_internal.h"
#include "vcrui.h"
#include "log.h"
#include <assert.h>

vcrui_t *vcrui_create(memory_arena_handle_t *memory_arena_handle)
{
	vcrui_t *handle = memory_arena_push(memory_arena_handle, sizeof(struct vcrui));
	assert(handle);
	handle->initialised = true;

	handle->window_size.x = 674;
	handle->window_size.y = 150;

	GLOG_INFO(LS_VCRUI, "vcrui initialised");

	return handle;
}

void vcrui_destroy(vcrui_t *handle)
{
	assert(handle->initialised);
	handle->initialised = false;
	GLOG_INFO(LS_VCRUI, "vcrui destroyed");
}

void vcrui_draw(vcrui_t *handle, struct game_state *game_state)
{
	assert(handle->initialised);

	rlImGuiBegin();

	igSetNextWindowSize((ImVec2){handle->window_size.x, handle->window_size.y},
						ImGuiCond_FirstUseEver);

	igBegin("VCR UI", NULL, 0);

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
	if (igButton("RESUME", (ImVec2){100, 40})) {
		time_machine_set_resume_position(tm, RP_CURRENT, game_state);
		game_state->vars.mode = GM_TICK;
	}

	igEnd();

	rlImGuiEnd();
}
