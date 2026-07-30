#ifndef EVENTS_H
#define EVENTS_H

#include "memory/memory.h"

#define MAX_LISTENERS_PER_EVENT_TYPE 10

// Forward declaration
struct game_state;

enum event_type {
	EVT_GOAL_SCORED,
	EVT_BALL_OOB_NORTH,
	EVT_BALL_OOB_SOUTH,
	EVT_BALL_OOB_EAST,
	EVT_BALL_OOB_WEST,
	// Debug tuning changed (a game_constants field was edited). Interested parties (e.g. players
	// recomputing their resolved constants) subscribe to refresh cached derived values.
	EVT_CONSTANTS_CHANGED,
	EVT_COUNT
};

extern const char *event_type_to_string[EVT_COUNT];

typedef struct events_router events_router_t;

struct events_router *events_router_create(memory_arena_handle_t *memory_arena_handle);
void events_router_destroy(events_router_t *handle);

void events_router_add_listener(events_router_t *handle,
								void (*listener)(enum event_type, void *),
								usize count,
								...);
void events_router_publish(events_router_t *handle, enum event_type event_type, void *context);

#endif // !EVENTS_H
