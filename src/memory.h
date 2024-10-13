#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

#define DEFAULT_MEMORY_ALIGNMENT 16

struct memory_arena_push_settings
{
    u16 alignment;
    b32 clear_memory;
};

struct memory_fixed_allocation
{
    b32 used;
    byte buffer[];
};

typedef struct memory_arena_handle memory_arena_handle_t;

memory_arena_handle_t *memory_arena_create(char *name, void *buffer, usize size);
void memory_arena_destroy(memory_arena_handle_t *handle);
void memory_arena_reset(memory_arena_handle_t *handle);
void *memory_arena_push(memory_arena_handle_t *handle, usize size);
void *memory_arena_push_string(memory_arena_handle_t *handle, char *string);
void *memory_arena_push_with_custom_settings(memory_arena_handle_t *handle, usize size,
					     struct memory_arena_push_settings settings);

typedef struct memory_fixed_handle memory_fixed_handle_t;

memory_fixed_handle_t *memory_fixed_create(char *name, usize size, usize allocations,
					   u16 alignment);
void memory_fixed_destroy(memory_fixed_handle_t *handle);
void *memory_fixed_alloc(memory_fixed_handle_t *handle);
void memory_fixed_free(memory_fixed_handle_t *handle, void *buffer);

usize memory_utils_align(usize to_align, u16 alignment);

#endif // !MEMORY_H
