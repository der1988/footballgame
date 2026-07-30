#include "memory/internal/memory.h"
#include "memory/memory.h"
#include "types.h"
#include "unity/unity.h"
#include "unity/unity_internals.h"
#include "log/log.h"
#include <assert.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setUp(void) {}

void tearDown(void) {}

void test_memory_arena()
{
	GLOG_INFO(LS_TESTS, "memory arena test - start");

	void *buffer = malloc(Megabytes(1));
	// valid creation
	memory_arena_handle_t *handle = memory_arena_create_default("test_arena", buffer, Megabytes(1));
	TEST_ASSERT_NOT_NULL(handle);
	TEST_ASSERT_EQUAL_INT(0, strcmp(handle->name, "test_arena"));
	TEST_ASSERT_EQUAL_UINT64(Megabytes(1), handle->total_size);

	// valid allocation
	void *buffer_1 = memory_arena_push(handle, Kilobytes(100));
	TEST_ASSERT_NOT_NULL(buffer_1);
	// allocate over remaning memory
	void *buffer_2 = memory_arena_push(handle, Megabytes(1));
	TEST_ASSERT_NULL(buffer_2);
	// valid allocation
	void *buffer_3 = memory_arena_push(handle, Kilobytes(500));
	TEST_ASSERT_NOT_NULL(buffer_3);
	// push string
	void *string = memory_arena_push_string(handle, "test_string");
	TEST_ASSERT_NOT_NULL(string);
	TEST_ASSERT_EQUAL_INT(0, strcmp(string, "test_string"));
	// reset and allocate max
	memory_arena_reset(handle);
	void *buffer_4 = memory_arena_push(handle, Megabytes(1));
	TEST_ASSERT_NOT_NULL(buffer_4);
	// test pop
	memory_arena_reset(handle);
	void *buffer_after_push = memory_arena_push(handle, Kilobytes(1));
	void *buffer_after_pop = memory_arena_pop(handle, Kilobytes(1));
	TEST_ASSERT_EQUAL_PTR(buffer_after_push, buffer_after_pop);
	// test pop too much
	buffer_after_pop = memory_arena_pop(handle, Kilobytes(1));
	TEST_ASSERT_NULL(buffer_after_pop);

	// valid destruction
	memory_arena_destroy(handle);
	handle = NULL;

	// invalid allocation on destroyed arena
	void *buffer_5 = memory_arena_push(handle, Kilobytes(100));
	TEST_ASSERT_NULL(buffer_5);

	// double destroy arena
	memory_arena_destroy(handle);

	GLOG_INFO(LS_TESTS, "memory arena test - end");
}

void test_memory_fixed()
{
	GLOG_INFO(LS_TESTS, "fixed size linear allocator test - start");

	// valid creation
	memory_fixed_handle_t *handle = memory_fixed_create("test_fixed", 100, 3, 4);
	TEST_ASSERT_NOT_NULL(handle);
	TEST_ASSERT_EQUAL_INT(0, strcmp(handle->name, "test_fixed"));
	TEST_ASSERT_EQUAL_UINT64(108, handle->fixed_allocation_size);
	TEST_ASSERT_EQUAL_UINT64(3, handle->allocations);
	TEST_ASSERT_EQUAL_UINT64(0, handle->allocated);

	// valid allocation
	void *buffer_1 = memory_fixed_alloc(handle);
	TEST_ASSERT_NOT_NULL(buffer_1);

	// valid allocation
	void *buffer_2 = memory_fixed_alloc(handle);
	TEST_ASSERT_NOT_NULL(buffer_2);

	// valid allocation
	void *buffer_3 = memory_fixed_alloc(handle);
	TEST_ASSERT_NOT_NULL(buffer_3);

	// invalid allocation
	void *buffer_4 = memory_fixed_alloc(handle);
	TEST_ASSERT_NULL(buffer_4);

	memory_fixed_free(handle, buffer_2);

	// should be valid now
	buffer_4 = memory_fixed_alloc(handle);
	TEST_ASSERT_NOT_NULL(buffer_4);

	// valid destruction
	memory_fixed_destroy(handle);
	handle = NULL;

	// invalid allocation on destroyed allocator
	void *buffer_5 = memory_fixed_alloc(handle);
	TEST_ASSERT_NULL(buffer_5);

	// double destroy arena
	memory_fixed_destroy(handle);

	// too much memory requested
	memory_fixed_handle_t *handle_impossible =
		memory_fixed_create("test_fixed", Gigabytes(128), 1, 4);
	TEST_ASSERT_NULL(handle_impossible);

	GLOG_INFO(LS_TESTS, "fixed size linear allocator test - end");
}

int main()
{
	UNITY_BEGIN();

	RUN_TEST(test_memory_arena);
	RUN_TEST(test_memory_fixed);

	return UNITY_END();
}
