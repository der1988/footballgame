#ifndef ASSET_ANIMATION_CACHE_INTERNAL_H
#define ASSET_ANIMATION_CACHE_INTERNAL_H

#include "types.h"
#include "memory.h"

struct assets_animation_cache {
	b32 initialised;
	memory_arena_handle_t *memory_arena_handle;
	struct assets_animation *cache;
	u32 count;
};

#endif // !ASSET_ANIMATION_CACHE_INTERNAL_H
