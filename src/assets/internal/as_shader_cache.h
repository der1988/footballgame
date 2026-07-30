#ifndef ASSET_SHADER_CACHE_INTERNAL_H
#define ASSET_SHADER_CACHE_INTERNAL_H

#include "types.h"
#include "memory/memory.h"

struct asset_shader_cache {
	b32 initialised;
	memory_arena_handle_t *memory_arena_handle;
	struct asset_shader *cache;
	u32 count;
};

#endif // !ASSET_SHADER_CACHE_INTERNAL_H
