#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

#define DEFAULT_MEMORY_ALIGNMENT 16

struct MemoryArenaPushSettings
{
    u16 alignment;
    b32 clear_memory;
};

struct MemoryFixedAllocation
{
    b32 used;
    byte buffer[];
};

typedef struct MemoryArenaHandle MemoryArenaHandle;

MemoryArenaHandle *memory_arena_create(char *name, void *buffer, usize size);
void memory_arena_destroy(MemoryArenaHandle *handle);
void memory_arena_reset(MemoryArenaHandle *handle);
void *memory_arena_push(MemoryArenaHandle *handle, usize size);
void *memory_arena_push_string(MemoryArenaHandle *handle, char *string);
void *memory_arena_push_with_custom_settings(MemoryArenaHandle *handle, usize size,
					     struct MemoryArenaPushSettings settings);

typedef struct MemoryFixedHandle MemoryFixedHandle;

MemoryFixedHandle *memory_fixed_create(char *name, usize size, usize allocations, u16 alignment);
void memory_fixed_destroy(MemoryFixedHandle *handle);
void *memory_fixed_alloc(MemoryFixedHandle *handle);
void memory_fixed_free(MemoryFixedHandle *handle, void *buffer);

usize memory_utils_align(usize to_align, u16 alignment);

#endif // !MEMORY_H
