#include "memory_internal.h"
#include "memory.h"
#include "types.h"
#include "log.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Create a fixed sized linear allocator
memory_fixed_handle_t *memory_fixed_create(char *name, usize size, usize allocations, u16 alignment)
{
    size = memory_utils_align(size, alignment);
    usize real_size = size + sizeof(b32);
    usize total_size = real_size * allocations;
    void *buffer;
    if ((buffer = malloc(total_size)) == NULL) {
	int errsv = errno;
	GLOGF_ERROR_CTX(LS_MEMORY,
			name,
			"failed allocating memory for fixed size linear allocator: %s",
			strerror(errsv));
	return NULL;
    };
    memory_fixed_handle_t *handle = malloc(sizeof(memory_fixed_handle_t));
    handle->name = name;
    handle->base = handle->current = buffer;
    memset(buffer, 0, total_size);
    handle->fixed_allocation_size = real_size;
    handle->allocations = allocations;
    handle->allocated = 0;
    GLOGF_INFO_CTX(LS_MEMORY,
		   name,
		   "fixed size linear allocator created for %zu allocations of size %zu bytes",
		   allocations,
		   size);
    return handle;
}

// Destroy a fixed memory allocator
void memory_fixed_destroy(memory_fixed_handle_t *handle)
{
    if (handle) {
	free(handle->base);
	GLOG_INFO_CTX(LS_MEMORY, handle->name, "fixed size linear allocator destroyed");
	free(handle);
	handle = NULL;
    } else {
	GLOG_ERROR(LS_MEMORY, "attempt to free an invalid fixed size linear allocator");
    }
}

void memory_fixed_reset(memory_arena_handle_t *handle)
{
    if (handle) {
	GLOG_TRACE_CTX(LS_MEMORY, handle->name, "fixed size linear allocator reset");
	handle->current = handle->base;
    } else {
	GLOG_ERROR(LS_MEMORY, "attempt to reset an invalid fixed size linear allocator");
    }
}

// Allocate fixed size buffer using the linear allocator
void *memory_fixed_alloc(memory_fixed_handle_t *handle)
{
    if (handle) {
	if (handle->allocated < handle->allocations) {
	    struct memory_fixed_allocation *allocation;
	    while ((allocation = (struct memory_fixed_allocation *)handle->current)->used) {
		handle->current += handle->fixed_allocation_size;
		if (handle->current ==
		    (handle->base + handle->fixed_allocation_size * handle->allocations)) {
		    handle->current = handle->base;
		}
	    }
	    allocation->used = true;
	    handle->allocated++;
	    GLOGF_INFO_CTX(LS_MEMORY,
			   handle->name,
			   "allocated %lu bytes from fixed size linear allocator, remaining %zu "
			   "allocations out of %zu",
			   handle->fixed_allocation_size - sizeof(b32),
			   handle->allocations - handle->allocated,
			   handle->allocations);
	    return &allocation->buffer;
	} else {
	    GLOG_ERROR_CTX(
		LS_MEMORY, handle->name, "not enough memory left in fixed size linear allocator");
	}
    } else {
	GLOG_ERROR(LS_MEMORY,
		   "attempt to allocate memory from invalid fixed size linear allocator");
    }
    return NULL;
}

// Free fixed size buffer previous allocated with the linear allocator
void memory_fixed_free(memory_fixed_handle_t *handle, void *buffer)
{
    if (handle) {
	struct memory_fixed_allocation *allocation = buffer - sizeof(b32);
	allocation->used = false;
	handle->allocated--;
	GLOGF_INFO_CTX(LS_MEMORY,
		       handle->name,
		       "freed %lu bytes from fixed size linear allocator, remaining %zu "
		       "allocations out of %zu",
		       handle->fixed_allocation_size - sizeof(b32),
		       handle->allocations - handle->allocated,
		       handle->allocations);
    } else {
	GLOG_ERROR(LS_MEMORY, "attempt to free memory from invalid fixed size linear allocator");
    }
}
