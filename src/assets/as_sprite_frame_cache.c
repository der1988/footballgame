#include "../assets/assets.h"
#include "../memory/memory.h"
#include "../utils/utils.h"
#include "libs/cJSON/cJSON.h"
#include "types.h"
#include <assert.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED_PARAM __attribute__((unused))

static u32 const MAX_SPRITE_FRAMES = 2000;

AssetsSpriteFrameCache *assets_sprite_frame_cache_create(MemoryArenaHandle *memory_arena_handle) {
    AssetsSpriteFrameCache *handle =
        memory_arena_push(memory_arena_handle, sizeof(AssetsSpriteFrameCache));
    assert(handle);

    handle->memory_arena_handle = memory_arena_handle;
    handle->cache =
        memory_arena_push(memory_arena_handle, sizeof(AssetsSpriteFrame) * MAX_SPRITE_FRAMES);
    handle->count = 0;
    if (handle->cache == NULL) {
        GLOG_ERROR(LS_ASSETS, "sprite frame cache initialisation failed");
        return false;
    }
    handle->initialised = true;

    GLOGF_INFO(LS_ASSETS, "sprite frame cache initialised for %d sprite frames", MAX_SPRITE_FRAMES);

    return handle;
}

void assets_sprite_frame_cache_destroy(AssetsSpriteFrameCache *handle) {
    assert(handle->initialised);
    handle->cache = NULL;
    handle->count = 0;
    handle->initialised = false;
    GLOG_INFO(LS_ASSETS, "sprite frame cache destroyed");
}

i32 assets_sprite_frames_load_from_json(AssetsSpriteFrameCache *handle, const u32 texture_id,
                                        const char *filename) {
    assert(handle->initialised);

    usize loaded = 0;
    AssetsSpriteFrame *sprite_frames =
        assets_texture_packer_load_from_json(filename, texture_id, handle->count, &loaded);
    if (loaded > (MAX_SPRITE_FRAMES - handle->count)) {
        GLOG_ERROR(LS_ASSETS, "sprite frame cache is too small");
    } else if (sprite_frames) {
        memcpy(&handle->cache[handle->count], sprite_frames, sizeof(AssetsSpriteFrame) * loaded);
        free(sprite_frames);
        handle->count += loaded;
        GLOGF_INFO_CTX(LS_ASSETS, filename, "loaded %zu sprite frames into cache, texture id is %d",
                       loaded, texture_id);
        return loaded;
    } else {
        GLOG_ERROR(LS_ASSETS, "failed loading sprite frames from json");
    }

    return -1;
}

AssetsSpriteFrame *assets_sprite_frame_get_by_name(AssetsSpriteFrameCache *handle,
                                                   const char *name) {
    assert(handle->initialised);

    AssetsSpriteFrame *sprite_frame = NULL;
    for (usize i = 0; i < handle->count; ++i) {
        sprite_frame = &handle->cache[i];
        if (strcmp(sprite_frame->name, name) == 0) {
            return sprite_frame;
        }
    }
    return NULL;
}

AssetsSpriteFrame *assets_sprite_frame_get_by_id(AssetsSpriteFrameCache *handle,
                                                 const AssetsSpriteFrameId id) {
    assert(handle->initialised);
    return &handle->cache[id];
}
