#include "memory_internal.h"
#include "memory.h"
#include "types.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Create an arena linear memory allocator using a specific buffer
memory_arena_handle_t *memory_arena_create(char *name, void *buffer, usize size)
{
    memory_arena_handle_t *handle = malloc(sizeof(memory_arena_handle_t));
    handle->name = name;
    handle->base = handle->current = buffer;
    handle->total_size = size;
    GLOGF_INFO_CTX(LS_MEMORY, name, "memory arena created with size %zu", size);
    return handle;
}

// Destroy an arena linear allocator
void memory_arena_destroy(memory_arena_handle_t *handle)
{
    if (handle)
    {
	GLOG_INFO_CTX(LS_MEMORY, handle->name, "memory arena destroyed");
	free(handle);
	handle = NULL;
    }
    else
    {
	GLOG_ERROR(LS_MEMORY, "attempt to free an invalid memory arena");
    }
}

void memory_arena_reset(memory_arena_handle_t *handle)
{
    if (handle)
    {
	GLOG_TRACE_CTX(LS_MEMORY, handle->name, "memory arena reset");
	handle->current = handle->base;
    }
    else
    {
	GLOG_ERROR(LS_MEMORY, "attempt to reset an invalid memory arena");
    }
}

// Allocate some memory inside the arena
void *memory_arena_push_with_custom_settings(memory_arena_handle_t *handle, usize size,
					     struct memory_arena_push_settings settings)
{
    if (handle)
    {
	size = memory_utils_align(size, settings.alignment);
	u64 remaining = handle->total_size - (handle->current - handle->base);
	if (size <= remaining)
	{
	    void *buffer = handle->current;
	    if (settings.clear_memory)
	    {
		memset(buffer, 0, size);
	    }
	    handle->current += size;
	    GLOGF_TRACE_CTX(LS_MEMORY, handle->name,
			    "allocated %lu bytes from memory arena, remaining %lu bytes", size,
			    remaining - size);
	    return buffer;
	}
	else
	{
	    GLOGF_ERROR_CTX(LS_MEMORY, handle->name,
			    "not enough memory left in memory arena, allocation of %lu "
			    "bytes "
			    "failed",
			    size);
	}
    }
    else
    {
	GLOG_ERROR(LS_MEMORY, "attempt to allocate memory from invalid arena");
    }
    return NULL;
}

// Allocate some memory inside the arena
void *memory_arena_push(memory_arena_handle_t *handle, usize size)
{
    struct memory_arena_push_settings settings = (struct memory_arena_push_settings){
	.alignment = DEFAULT_MEMORY_ALIGNMENT, .clear_memory = true};
    return memory_arena_push_with_custom_settings(handle, size, settings);
}

void *memory_arena_push_string(memory_arena_handle_t *handle, char *string)
{
    char *new_string = memory_arena_push(handle, strlen(string) + 1);
    usize i;
    for (i = 0; i < strlen(string); ++i)
    {
	new_string[i] = string[i];
    }
    new_string[i] = 0;
    return new_string;
}
