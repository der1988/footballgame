#ifndef ASSET_SPRITE_FRAME_CACHE_INTERNAL_H
#define ASSET_SPRITE_FRAME_CACHE_INTERNAL_H

#include "types.h"
#include "memory/memory.h"

struct asset_sprite_frame_cache {
	b32 initialised;
	memory_arena_handle_t *memory_arena_handle;
	struct asset_sprite_frame *cache;
	u32 count;
};

#endif // !ASSET_SPRITE_FRAME_CACHE_INTERNAL_H
