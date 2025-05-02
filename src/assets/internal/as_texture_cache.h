#ifndef ASSET_TEXTURE_CACHE_INTERNAL_H
#define ASSET_TEXTURE_CACHE_INTERNAL_H

#include "types.h"

struct asset_texture_cache {
	b32 initialised;
	struct asset_texture *cache;
	u32 count;
};

#endif // !ASSET_TEXTURE_CACHE_INTERNAL_H
