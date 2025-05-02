#ifndef ASSET_TEXTURE_H
#define ASSET_TEXTURE_H

#include "types.h"
#include "memory.h"

#define TEXTURE_NAME_MAX_SIZE 50

// Texture Cache
typedef u32 assets_texture_id_t;

typedef struct assets_texture_cache assets_texture_cache_t;

struct assets_texture {
	assets_texture_id_t id;
	char name[TEXTURE_NAME_MAX_SIZE];
	texture_t texture;
};

struct assets_texture_cache *assets_texture_cache_create(memory_arena_handle_t *memory_arena_handle);
void assets_texture_cache_destroy(assets_texture_cache_t *handle);
struct assets_texture *assets_texture_load(assets_texture_cache_t *handle, const char *filename);
struct assets_texture *assets_texture_get_by_id(assets_texture_cache_t *handle, const u32 id);
struct assets_texture *assets_texture_get_by_name(assets_texture_cache_t *handle,
												  const char *filename);

#endif // !ASSET_TEXTURE_H
