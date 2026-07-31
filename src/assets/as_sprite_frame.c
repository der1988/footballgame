#include "assets/as_sprite_frame.h"
#include "memory/memory.h"
#include "types.h"
#include "log.h"
#include "assets/texture_packer_loader.h"
#include <assert.h>
#include <string.h>

struct asset_sprite_frame_cache {
	b32 initialised;
	memory_arena_handle_t *memory_arena_handle;
	struct asset_sprite_frame *cache;
	u32 count;
};

static u32 const MAX_SPRITE_FRAMES = 2000;

asset_sprite_frame_cache_t *
asset_sprite_frame_cache_create(memory_arena_handle_t *memory_arena_handle)
{
	asset_sprite_frame_cache_t *handle =
		memory_arena_push(memory_arena_handle, sizeof(asset_sprite_frame_cache_t));
	assert(handle);

	handle->memory_arena_handle = memory_arena_handle;
	handle->cache = memory_arena_push(memory_arena_handle,
									  sizeof(struct asset_sprite_frame) * MAX_SPRITE_FRAMES);
	handle->count = 0;
	if (handle->cache == NULL) {
		GLOG_ERROR(LS_ASSETS, "sprite frame cache initialisation failed");
		return false;
	}
	handle->initialised = true;

	GLOGF_INFO(LS_ASSETS, "sprite frame cache initialised for %d sprite frames", MAX_SPRITE_FRAMES);

	return handle;
}

void asset_sprite_frame_cache_destroy(asset_sprite_frame_cache_t *handle)
{
	assert(handle->initialised);
	handle->cache = NULL;
	handle->count = 0;
	handle->initialised = false;
	GLOG_INFO(LS_ASSETS, "sprite frame cache destroyed");
}

size asset_sprite_frames_load_from_json(asset_sprite_frame_cache_t *handle,
									   const u32 texture_id,
									   const char *filename)
{
	assert(handle->initialised);

	size loaded = texture_packer_load_from_json(filename,
											   texture_id,
											   handle->count,
											   &handle->cache[handle->count],
											   MAX_SPRITE_FRAMES - handle->count);

	if (loaded >= 0) {
		handle->count += loaded;
		GLOGF_INFO_CTX(LS_ASSETS,
					   filename,
					   "loaded %d sprite frames into cache, texture id is %d",
					   loaded,
					   texture_id);
	} else {
		GLOG_ERROR(LS_ASSETS, "failed loading sprite frames from json");
	}

	return loaded;
}

struct asset_sprite_frame *asset_sprite_frame_get_by_name(asset_sprite_frame_cache_t *handle,
														  const char *name)
{
	assert(handle->initialised);

	struct asset_sprite_frame *sprite_frame = NULL;
	for (usize i = 0; i < handle->count; ++i) {
		sprite_frame = &handle->cache[i];
		if (strcmp(sprite_frame->name, name) == 0) {
			return sprite_frame;
		}
	}
	return NULL;
}

struct asset_sprite_frame *asset_sprite_frame_get_by_id(asset_sprite_frame_cache_t *handle,
														const asset_sprite_frame_id_t id)
{
	assert(handle->initialised);
	return &handle->cache[id];
}
