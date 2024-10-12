#ifndef MEMORY_INTERNAL_H
#define MEMORY_INTERNAL_H

#include "types.h"

struct MemoryArenaHandle
{
    char *name;
    void *base;
    void *current;
    usize total_size;
};

struct MemoryFixedHandle
{
    char *name;
    void *base;
    void *current;
    usize fixed_allocation_size;
    usize allocations;
    usize allocated;
};

#endif
