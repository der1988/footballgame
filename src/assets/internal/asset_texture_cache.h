#ifndef ASSET_TEXTURE_CACHE_INTERNAL_H
#define ASSET_TEXTURE_CACHE_INTERNAL_H

#include "types.h"

struct assets_texture_cache {
	b32 initialised;
	struct assets_texture *cache;
	u32 count;
};

#endif // !ASSET_TEXTURE_CACHE_INTERNAL_H
