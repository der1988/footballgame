#include "game.h"
#include "time_machine.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui/cimgui.h"
#include "debugui_internal.h"
#include "debugui.h"
#include <assert.h>

#ifdef HAS_FFMPEG
#include "video_export/video_export.h"
#endif

static void draw_marker_on_slider(ImDrawList *draw_list,
								  ImVec2_c slider_min,
								  ImVec2_c slider_max,
								  i32 marker_frame,
								  i32 max_frame,
								  ImU32 color)
{
	if (marker_frame < 0 || max_frame <= 0)
		return;

	f32 t = (f32)marker_frame / (f32)max_frame;
	f32 x = slider_min.x + t * (slider_max.x - slider_min.x);

	ImVec2_c top = {x, slider_min.y};
	ImVec2_c bottom = {x, slider_max.y};
	ImDrawList_AddLine(draw_list, top, bottom, color, 2.0f);
}

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

#ifdef HAS_FFMPEG

		// Draw markers on the slider
		ImDrawList *draw_list = igGetWindowDrawList();
		ImVec2_c slider_min = igGetItemRectMin();
		ImVec2_c slider_max = igGetItemRectMax();

		ImU32 green = igColorConvertFloat4ToU32((ImVec4_c){0.0f, 1.0f, 0.0f, 0.9f});
		ImU32 red = igColorConvertFloat4ToU32((ImVec4_c){1.0f, 0.0f, 0.0f, 0.9f});
		draw_marker_on_slider(
			draw_list, slider_min, slider_max, handle->vcr_export_start_marker, max_frame, green);
		draw_marker_on_slider(
			draw_list, slider_min, slider_max, handle->vcr_export_end_marker, max_frame, red);

		// Marker buttons
		if (igButton("Mark Start", (ImVec2){0, 0})) {
			handle->vcr_export_start_marker = cur_frame;
		}
		igSameLine(0, 8);
		if (igButton("Mark End", (ImVec2){0, 0})) {
			handle->vcr_export_end_marker = cur_frame;
		}
		igSameLine(0, 16);

		// Show marker info
		if (handle->vcr_export_start_marker >= 0 && handle->vcr_export_end_marker >= 0) {
			igText("Start: %d  End: %d  (%d frames)",
				   handle->vcr_export_start_marker,
				   handle->vcr_export_end_marker,
				   handle->vcr_export_end_marker - handle->vcr_export_start_marker + 1);
		} else if (handle->vcr_export_start_marker >= 0) {
			igText("Start: %d  End: not set", handle->vcr_export_start_marker);
		} else if (handle->vcr_export_end_marker >= 0) {
			igText("Start: not set  End: %d", handle->vcr_export_end_marker);
		} else {
			igText("No markers set");
		}

		igInputText(
			"Filename", handle->vcr_export_filename, VCR_EXPORT_FILENAME_BUFFER_SIZE, 0, NULL, NULL);
		// Export button
		b32 can_export = handle->vcr_export_start_marker >= 0 &&
						 handle->vcr_export_end_marker >= 0 &&
						 handle->vcr_export_start_marker <= handle->vcr_export_end_marker;

		if (!can_export)
			igBeginDisabled(true);

		if (igButton("Export Video", (ImVec2){120, 30})) {
			video_export_range((usize)handle->vcr_export_start_marker,
							   (usize)handle->vcr_export_end_marker,
							   handle->vcr_export_filename,
							   game_state);
		}

		if (!can_export)
			igEndDisabled();
#endif
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
