#include "assets_internal.h"
#include "assets.h"
#include "memory.h"
#include "types.h"
#include "log.h"
#include <assert.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED_PARAM __attribute__((unused))

static u32 const MAX_SPRITE_FRAMES = 2000;

assets_sprite_frame_cache_t *
assets_sprite_frame_cache_create(memory_arena_handle_t *memory_arena_handle)
{
    assets_sprite_frame_cache_t *handle =
	memory_arena_push(memory_arena_handle, sizeof(assets_sprite_frame_cache_t));
    assert(handle);

    handle->memory_arena_handle = memory_arena_handle;
    handle->cache = memory_arena_push(memory_arena_handle,
				      sizeof(struct assets_sprite_frame) * MAX_SPRITE_FRAMES);
    handle->count = 0;
    if (handle->cache == NULL) {
	GLOG_ERROR(LS_ASSETS, "sprite frame cache initialisation failed");
	return false;
    }
    handle->initialised = true;

    GLOGF_INFO(LS_ASSETS, "sprite frame cache initialised for %d sprite frames", MAX_SPRITE_FRAMES);

    return handle;
}

void assets_sprite_frame_cache_destroy(assets_sprite_frame_cache_t *handle)
{
    assert(handle->initialised);
    handle->cache = NULL;
    handle->count = 0;
    handle->initialised = false;
    GLOG_INFO(LS_ASSETS, "sprite frame cache destroyed");
}

i32 assets_sprite_frames_load_from_json(assets_sprite_frame_cache_t *handle,
					const u32 texture_id,
					const char *filename)
{
    assert(handle->initialised);

    usize loaded = 0;
    struct assets_sprite_frame *sprite_frames =
	assets_texture_packer_load_from_json(filename, texture_id, handle->count, &loaded);
    assert(loaded <= (MAX_SPRITE_FRAMES - handle->count));

    if (sprite_frames) {
	memcpy(&handle->cache[handle->count],
	       sprite_frames,
	       sizeof(struct assets_sprite_frame) * loaded);
	free(sprite_frames);
	handle->count += loaded;
	GLOGF_INFO_CTX(LS_ASSETS,
		       filename,
		       "loaded %zu sprite frames into cache, texture id is %d",
		       loaded,
		       texture_id);
    } else {
	loaded = -1;
	GLOG_ERROR(LS_ASSETS, "failed loading sprite frames from json");
    }

    return loaded;
}

struct assets_sprite_frame *assets_sprite_frame_get_by_name(assets_sprite_frame_cache_t *handle,
							    const char *name)
{
    assert(handle->initialised);

    struct assets_sprite_frame *sprite_frame = NULL;
    for (usize i = 0; i < handle->count; ++i) {
	sprite_frame = &handle->cache[i];
	if (strcmp(sprite_frame->name, name) == 0) {
	    return sprite_frame;
	}
    }
    return NULL;
}

struct assets_sprite_frame *assets_sprite_frame_get_by_id(assets_sprite_frame_cache_t *handle,
							  const assets_sprite_frame_id_t id)
{
    assert(handle->initialised);
    return &handle->cache[id];
}
