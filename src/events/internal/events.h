#ifndef EVENTS_INTERNAL_H
#define EVENTS_INTERNAL_H

#include "memory.h"
#include "types.h"
#include "../events.h"

struct list_listeners {
	void (**array)(enum event_type, void *);
	usize count;
	usize size;
};

struct events_router {
	memory_arena_handle_t *memory_arena_handle;
	struct list_listeners listeners[EVT_COUNT];
	b32 initialised;
};
#endif // !EVENTS_INTERNAL_H
