#ifndef ASSET_ANIMATION_CACHE_INTERNAL_H
#define ASSET_ANIMATION_CACHE_INTERNAL_H

#include "types.h"
#include "memory/memory.h"

struct asset_animation_cache {
	b32 initialised;
	memory_arena_handle_t *memory_arena_handle;
	struct asset_animation *cache;
	u32 count;
};

#endif // !ASSET_ANIMATION_CACHE_INTERNAL_H
