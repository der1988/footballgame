#ifndef MEMORY_INTERNAL_H
#define MEMORY_INTERNAL_H

#include "types.h"

struct memory_arena_handle
{
    char *name;
    void *base;
    void *current;
    usize total_size;
};

struct memory_fixed_handle
{
    char *name;
    void *base;
    void *current;
    usize fixed_allocation_size;
    usize allocations;
    usize allocated;
};

#endif
