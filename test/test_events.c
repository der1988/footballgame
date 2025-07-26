#include "memory.h"
#include "types.h"
#include "unity/unity.h"
#include "unity/unity_internals.h"
#include "log.h"
#include "events/events.h"
#include <assert.h>
#include <raylib.h>
#include <stdlib.h>

void setUp(void) {}

void tearDown(void) {}

struct test_context {
	u32 value;
};

static u32 listener_called_times = 0;

void test_listener(enum event_type event_type, void *context)
{
	assert(event_type == EVT_GOAL_SCORED);
	assert(((struct test_context *)context)->value == 69);
	listener_called_times++;
}

void test_events()
{
	GLOG_INFO(LS_TESTS, "events test - start");

	void *buffer = malloc(Megabytes(1));
	// valid creation
	memory_arena_handle_t *handle = memory_arena_create_default("test_arena", buffer, Megabytes(1));

	struct events_router *events_router = events_router_create(handle);
	events_router_add_listener(events_router, EVT_GOAL_SCORED, test_listener);

	struct test_context test_context = {.value = 69};

	events_router_publish(events_router, EVT_GOAL_SCORED, &test_context);

	assert(listener_called_times == 1);

	events_router_add_listener(events_router, EVT_GOAL_SCORED, test_listener);
	events_router_publish(events_router, EVT_GOAL_SCORED, &test_context);

	assert(listener_called_times == 3);

	// double destroy arena
	memory_arena_destroy(handle);

	GLOG_INFO(LS_TESTS, "events test - end");
}

int main()
{
	UNITY_BEGIN();

	RUN_TEST(test_events);

	return UNITY_END();
}
