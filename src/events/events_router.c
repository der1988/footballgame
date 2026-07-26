#include "internal/events.h"
#include "log.h"
#include "macro.h"
#include <assert.h>
#include <stdarg.h>

const char *event_type_to_string[EVT_COUNT] = {"goal_scored",
												"ball_oob_north",
												"ball_oob_south",
												"ball_oob_east",
												"ball_oob_west",
												"constants_changed"};

struct events_router *events_router_create(memory_arena_handle_t *memory_arena_handle)
{
	assert(memory_arena_handle);

	struct events_router *handle =
		memory_arena_push(memory_arena_handle, sizeof(struct events_router));
	assert(handle);

	handle->memory_arena_handle = memory_arena_handle;
	for (usize i = 0; i < EVT_COUNT; ++i) {
		void *listeners_array =
			memory_arena_push(memory_arena_handle,
							  sizeof(void (*)(enum event_type, void *, struct game_state *)) *
								  MAX_LISTENERS_PER_EVENT_TYPE);
		assert(listeners_array);
		handle->listeners[i] = (struct list_listeners){
			.count = 0, .size = MAX_LISTENERS_PER_EVENT_TYPE, .array = listeners_array};
		handle->initialised = true;
	}

	GLOGF_INFO(LS_EVENTS, "events router initialised for %d event types", EVT_COUNT);

	return handle;
}

void events_router_destroy(struct events_router *handle)
{
	assert(handle->initialised);
	handle->initialised = false;
	GLOG_INFO(LS_EVENTS, "events router destroyed");
}

void events_router_add_listener(struct events_router *handle,
								void (*listener)(enum event_type, void *),
								usize count,
								...)
{
	va_list ap;
	va_start(ap, count);
	for (usize i = 0; i < count; ++i) {
		enum event_type event_type = (enum event_type)va_arg(ap, int);
		assert(event_type < EVT_COUNT);
		LIST_ADD(handle->listeners[event_type], listener);
		GLOGF_INFO(
			LS_EVENTS, "added listener for event type: %s", event_type_to_string[event_type]);
	}
	va_end(ap);
}

void events_router_publish(struct events_router *handle, enum event_type event_type, void *context)
{
	assert(event_type < EVT_COUNT);

	struct list_listeners *listeners_list = &handle->listeners[event_type];
	for (usize i = 0; i < listeners_list->count; ++i) {
		listeners_list->array[i](event_type, context);

		GLOGF_DEBUG(
			LS_EVENTS, "notified listener for event type: %s", event_type_to_string[event_type]);
	}

	GLOGF_INFO(LS_EVENTS, "publish complete for event type: %s", event_type_to_string[event_type]);
}
