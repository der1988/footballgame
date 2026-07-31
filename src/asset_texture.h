#ifndef ASSET_TEXTURE_H
#define ASSET_TEXTURE_H

#include "types.h"
#include "memory.h"

#define TEXTURE_NAME_MAX_SIZE 50

// Texture Cache
typedef u32 asset_texture_id_t;

typedef struct asset_texture_cache asset_texture_cache_t;

struct asset_texture {
	asset_texture_id_t id;
	char name[TEXTURE_NAME_MAX_SIZE];
	texture_t texture;
};

struct asset_texture_cache *asset_texture_cache_create(memory_arena_handle_t *memory_arena_handle);
void asset_texture_cache_destroy(asset_texture_cache_t *handle);
struct asset_texture *asset_texture_load(asset_texture_cache_t *handle, const char *filename);
struct asset_texture *asset_texture_get_by_id(asset_texture_cache_t *handle, const u32 id);
struct asset_texture *asset_texture_get_by_name(asset_texture_cache_t *handle,
												const char *filename);

#endif // !ASSET_TEXTURE_H
