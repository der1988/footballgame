#include "memory_internal.h"
#include "memory.h"
#include "types.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Create alinear memory allocator arena using a specific buffer
memory_arena_handle_t *
memory_arena_create(char *name, void *buffer, usize size, u16 alignment, b32 clear_memory)
{
	memory_arena_handle_t *handle = malloc(sizeof(memory_arena_handle_t));
	handle->name = name;
	handle->base = handle->current = buffer;
	handle->total_size = size;
	handle->alignment = alignment;
	handle->clear_memory = clear_memory;
	GLOGF_INFO_CTX(
		LS_MEMORY, name, "memory arena created with size %zu and %d bytes of alignment", size);
	return handle;
}

// Create an arena linear memory allocator using a specific buffer with default settings
memory_arena_handle_t *memory_arena_create_default(char *name, void *buffer, usize size)
{
	return memory_arena_create(name, buffer, size, DEFAULT_MEMORY_ALIGNMENT, true);
}

// Destroy a linear memory allocator arena
void memory_arena_destroy(memory_arena_handle_t *handle)
{
	if (handle) {
		GLOG_INFO_CTX(LS_MEMORY, handle->name, "memory arena destroyed");
		free(handle);
		handle = NULL;
	} else {
		GLOG_ERROR(LS_MEMORY, "attempt to free an invalid memory arena");
	}
}

void memory_arena_reset(memory_arena_handle_t *handle)
{
	if (handle) {
		GLOG_TRACE_CTX(LS_MEMORY, handle->name, "memory arena reset");
		handle->current = handle->base;
	} else {
		GLOG_ERROR(LS_MEMORY, "attempt to reset an invalid memory arena");
	}
}

// Allocate some memory inside the arena
void *memory_arena_push(memory_arena_handle_t *handle, usize size)
{
	if (handle) {
		size = memory_utils_align(size, handle->alignment);
		u64 remaining = handle->total_size - (handle->current - handle->base);
		if (size <= remaining) {
			void *buffer = handle->current;
			if (handle->clear_memory) {
				memset(buffer, 0, size);
			}
			handle->current += size;
			GLOGF_TRACE_CTX(LS_MEMORY,
							handle->name,
							"allocated %lu bytes from memory arena, remaining %lu bytes",
							size,
							remaining - size);
			return buffer;
		} else {
			GLOGF_ERROR_CTX(LS_MEMORY,
							handle->name,
							"not enough memory left in memory arena, allocation of %lu "
							"bytes "
							"failed",
							size);
		}
	} else {
		GLOG_ERROR(LS_MEMORY, "attempt to allocate memory from invalid arena");
	}
	return NULL;
}

// Deallocate some memory inside the arena
void *memory_arena_pop(memory_arena_handle_t *handle, usize size)
{
	if (handle) {
		size = memory_utils_align(size, handle->alignment);
		u64 used = handle->current - handle->base;
		if (size <= used) {
			void *buffer = handle->current;
			handle->current -= size;
			GLOGF_TRACE_CTX(LS_MEMORY,
							handle->name,
							"deallocated %lu bytes from memory arena, remaining %lu bytes",
							size,
							used - size);
			return buffer;
		} else {
			GLOGF_ERROR_CTX(LS_MEMORY,
							handle->name,
							"popping more than what's used in memory arena, deallocation of %lu "
							"bytes "
							"failed",
							size);
		}
	} else {
		GLOG_ERROR(LS_MEMORY, "attempt to deallocate memory from invalid arena");
	}
	return NULL;
}

// Allocate memory for a string
void *memory_arena_push_string(memory_arena_handle_t *handle, char *string)
{
	char *new_string = memory_arena_push(handle, strlen(string) + 1);
	usize i;
	for (i = 0; i < strlen(string); ++i) {
		new_string[i] = string[i];
	}
	new_string[i] = 0;
	return new_string;
}
