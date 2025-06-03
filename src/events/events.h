#ifndef EVENTS_H
#define EVENTS_H

#include "memory.h"

#define MAX_LISTENERS_PER_EVENT_TYPE 10

// Forward declaration
struct game_state;

enum event_type {
	EVT_GOAL_SCORED,
	EVT_COUNT
};

static const char *event_type_to_string[EVT_COUNT] = {"goal_scored"};

typedef struct events_router events_router_t;

struct events_router *events_router_create(memory_arena_handle_t *memory_arena_handle);
void events_router_destroy(events_router_t *handle);

void events_router_add_listener(events_router_t *handle,
								enum event_type event_type,
								void(listener)(enum event_type, void *));
void events_router_publish(events_router_t *handle, enum event_type event_type, void *context);

#endif // !EVENTS_H
