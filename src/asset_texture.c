#include "asset_texture.h"
#include "memory.h"
#include "types.h"
#include "log.h"
#include <assert.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct asset_texture_cache {
	b32 initialised;
	struct asset_texture *cache;
	u32 count;
};

static u32 const MAX_TEXTURES = 10;

asset_texture_cache_t *asset_texture_cache_create(memory_arena_handle_t *memory_arena_handle)
{
	asset_texture_cache_t *handle =
		memory_arena_push(memory_arena_handle, sizeof(asset_texture_cache_t));
	assert(handle);

	handle->cache =
		memory_arena_push(memory_arena_handle, sizeof(struct asset_texture) * MAX_TEXTURES);
	if (handle->cache == NULL) {
		GLOG_ERROR(LS_ASSETS, "texture cache initialisation failed");
		return false;
	}
	handle->initialised = true;
	GLOG_INFO(LS_ASSETS, "texture cache created");
	return handle;
}

struct asset_texture *asset_texture_load(asset_texture_cache_t *handle, const char *filename)
{
	struct asset_texture *assets_texture = NULL;
	assert(handle->initialised && handle->count < MAX_TEXTURES);
	Texture texture = {0};
	if ((texture = LoadTexture(filename)).id == 0) {
		GLOG_ERROR(LS_ASSETS, "error loading texture");
	} else {
		assets_texture = &handle->cache[handle->count];
		assets_texture->id = handle->count;
		assets_texture->texture = texture;
		strncpy(assets_texture->name, filename, TEXTURE_NAME_MAX_SIZE);
		handle->count++;
		GLOGF_INFO_CTX(LS_ASSETS,
					   filename,
					   "texture loaded with id %d and texture id %d",
					   assets_texture->id,
					   assets_texture->texture.id);
	}
	return assets_texture;
}

void asset_texture_cache_destroy(asset_texture_cache_t *handle)
{
	assert(handle->initialised);

	struct asset_texture *assets_texture = NULL;
	for (usize i = 0; i < handle->count; ++i) {
		assets_texture = &handle->cache[i];
		UnloadTexture(assets_texture->texture);
	}

	handle->cache = NULL;
	handle->count = 0;
	handle->initialised = false;
	GLOG_INFO(LS_ASSETS, "texture cache destroyed");
}

struct asset_texture *asset_texture_get_by_id(asset_texture_cache_t *handle, u32 id)
{
	assert(handle->initialised);
	struct asset_texture *assets_texture = &handle->cache[id];
	GLOGF_TRACE_CTX(LS_ASSETS,
					assets_texture->name,
					"retrieved texture with id %d and texture_id %d",
					assets_texture->id,
					assets_texture->texture.id);
	return assets_texture;
}

struct asset_texture *asset_texture_get_by_name(asset_texture_cache_t *handle, const char *filename)
{
	assert(handle->initialised);
	struct asset_texture *assets_texture = NULL;
	for (usize i = 0; i < handle->count; ++i) {
		assets_texture = &handle->cache[i];
		if (strcmp(assets_texture->name, filename) == 0) {
			break;
		}
	}
	GLOGF_TRACE_CTX(LS_ASSETS,
					filename,
					"retrieved texture with id %d and texture_id %d",
					assets_texture->id,
					assets_texture->texture.id);
	return assets_texture;
}
