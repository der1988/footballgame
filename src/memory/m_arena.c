#include "../utils/utils.h"
#include "memory.h"
#include "types.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Create an arena linear memory allocator using a specific buffer
MemoryArenaHandle *memory_arena_create(char *name, void *buffer, usize size) {
    MemoryArenaHandle *handle = malloc(sizeof(MemoryArenaHandle));
    handle->name = name;
    handle->base = handle->current = buffer;
    handle->total_size = size;
    GLOGF_INFO_CTX(LS_MEMORY, name, "memory arena created with size %zu", size);
    return handle;
}

// Destroy an arena linear allocator
void memory_arena_destroy(MemoryArenaHandle *handle) {
    if (handle) {
        GLOG_INFO_CTX(LS_MEMORY, handle->name, "memory arena destroyed");
        free(handle);
        handle = NULL;
    } else {
        GLOG_ERROR(LS_MEMORY, "attempt to free an invalid memory arena");
    }
}

void memory_arena_reset(MemoryArenaHandle *handle) {
    if (handle) {
        GLOG_TRACE_CTX(LS_MEMORY, handle->name, "memory arena reset");
        handle->current = handle->base;
    } else {
        GLOG_ERROR(LS_MEMORY, "attempt to reset an invalid memory arena");
    }
}

// Allocate some memory inside the arena
void *memory_arena_push_with_custom_settings(MemoryArenaHandle *handle, usize size,
                                             MemoryArenaPushSettings settings) {
    if (handle) {
        size = utils_align(size, settings.alignment);
        u64 remaining = handle->total_size - (handle->current - handle->base);
        if (size <= remaining) {
            void *buffer = handle->current;
            if (settings.clear_memory) {
                memset(buffer, 0, size);
            }
            handle->current += size;
            GLOGF_TRACE_CTX(LS_MEMORY, handle->name,
                            "allocated %lu bytes from memory arena, remaining %lu bytes", size,
                            remaining - size);
            return buffer;
        } else {
            GLOGF_ERROR_CTX(LS_MEMORY, handle->name,
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

// Allocate some memory inside the arena
void *memory_arena_push(MemoryArenaHandle *handle, usize size) {
    MemoryArenaPushSettings settings =
        (MemoryArenaPushSettings){.alignment = 4, .clear_memory = true};
    return memory_arena_push_with_custom_settings(handle, size, settings);
}

void *memory_arena_push_string(MemoryArenaHandle *handle, char *string) {
    char *new_string = memory_arena_push(handle, strlen(string) + 1);
    usize i;
    for (i = 0; i < strlen(string); ++i) {
        new_string[i] = string[i];
    }
    new_string[i] = 0;
    return new_string;
}
