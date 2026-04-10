#include "debugui_internal.h"
#include "debugui.h"
#include "log.h"
#include <assert.h>

debugui_t *debugui_create(memory_arena_handle_t *memory_arena_handle)
{
	debugui_t *handle = memory_arena_push(memory_arena_handle, sizeof(struct debugui));
	assert(handle);
	handle->initialised = true;

	handle->window_size.x = 674;
	handle->window_size.y = 426;
	handle->entity_inspector_selected_entity_id = EID_NOT_VALID;
	handle->entity_inspector_filter_buffer[0] = '\0';

	GLOG_INFO(LS_DEBUGUI, "debugui initialised");

	return handle;
}

void debugui_destroy(debugui_t *handle)
{
	assert(handle->initialised);
	handle->initialised = false;
	GLOG_INFO(LS_DEBUGUI, "debugui destroyed");
}
