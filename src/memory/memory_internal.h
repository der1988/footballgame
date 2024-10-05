#ifndef MEMORY_INTERNAL_H
#define MEMORY_INTERNAL_H

#include "types.h"

typedef struct MemoryArenaHandle MemoryArenaHandle;
struct MemoryArenaHandle {
    char *name;
    void *base;
    void *current;
    usize total_size;
};

typedef struct MemoryFixedHandle MemoryFixedHandle;
struct MemoryFixedHandle {
    char *name;
    void *base;
    void *current;
    usize fixed_allocation_size;
    usize allocations;
    usize allocated;
};

#endif
