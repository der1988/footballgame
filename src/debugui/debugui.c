#include "debugui_internal.h"
#include "debugui.h"
#include "log.h"
#include <assert.h>
#include <stdio.h>

debugui_t *debugui_create(memory_arena_handle_t *memory_arena_handle)
{
	debugui_t *handle = memory_arena_push(memory_arena_handle, sizeof(struct debugui));
	assert(handle);
	handle->initialised = true;

	handle->window_size.x = 674;
	handle->window_size.y = 426;
	handle->entity_inspector_selected_entity_id = EID_NOT_VALID;
	handle->entity_inspector_filter_buffer[0] = '\0';
	handle->vcr_export_start_marker = -1;
	handle->vcr_export_end_marker = -1;
	snprintf(handle->vcr_export_filename, VCR_EXPORT_FILENAME_BUFFER_SIZE, "export");

	GLOG_INFO(LS_DEBUGUI, "debugui initialised");

	return handle;
}

void debugui_destroy(debugui_t *handle)
{
	assert(handle->initialised);
	handle->initialised = false;
	GLOG_INFO(LS_DEBUGUI, "debugui destroyed");
}
