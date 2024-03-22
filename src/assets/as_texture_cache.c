#include "../assets/assets.h"
#include "../memory/memory.h"
#include "../utils/utils.h"
#include "types.h"
#include <assert.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED_PARAM __attribute__((unused))

static u32 const MAX_TEXTURES = 10;

AssetsTextureCache *assets_texture_cache_create(MemoryArenaHandle *memory_arena_handle) {
    AssetsTextureCache *handle = memory_arena_push(memory_arena_handle, sizeof(AssetsTextureCache));
    assert(handle);

    handle->cache = memory_arena_push(memory_arena_handle, sizeof(AssetsTexture) * MAX_TEXTURES);
    if (handle->cache == NULL) {
        GLOG_ERROR(LS_ASSETS, "texture cache initialisation failed");
        return false;
    }
    handle->initialised = true;
    GLOG_INFO(LS_ASSETS, "texture cache created");
    return handle;
}

AssetsTexture *assets_texture_load(AssetsTextureCache *handle, const char *filename) {
    AssetsTexture *assets_texture = NULL;
    assert(handle->initialised && handle->count < MAX_TEXTURES);
    Texture texture = {0};
    if ((texture = LoadTexture(filename)).id == 0) {
        GLOG_ERROR(LS_ASSETS, "error loading texture");
    } else {
        assets_texture = &handle->cache[handle->count];
        assets_texture->id = handle->count;
        assets_texture->texture = texture;
        strncpy(assets_texture->name, filename, 30);
        handle->count++;
        GLOGF_INFO_CTX(LS_ASSETS, filename, "texture loaded with id %d and texture id %d",
                       assets_texture->id, assets_texture->texture.id);
    }
    return assets_texture;
}

void assets_texture_cache_destroy(AssetsTextureCache *handle) {
    assert(handle->initialised);
    handle->cache = NULL;
    handle->count = 0;
    handle->initialised = false;
    GLOG_INFO(LS_ASSETS, "texture cache destroyed");
}

AssetsTexture *assets_texture_get_by_id(AssetsTextureCache *handle, u32 id) {
    assert(handle->initialised);
    AssetsTexture *assets_texture = &handle->cache[id];
    GLOGF_TRACE_CTX(LS_ASSETS, assets_texture->name,
                    "retrieved texture with id %d and texture_id %d", assets_texture->id,
                    assets_texture->texture.id);
    return assets_texture;
}

AssetsTexture *assets_texture_get_by_name(AssetsTextureCache *handle, const char *filename) {
    assert(handle->initialised);
    AssetsTexture *assets_texture = NULL;
    for (usize i = 0; i < handle->count; ++i) {
        assets_texture = &handle->cache[i];
        if (strcmp(assets_texture->name, filename) == 0) {
            break;
        }
    }
    GLOGF_TRACE_CTX(LS_ASSETS, filename, "retrieved texture with id %d and texture_id %d",
                    assets_texture->id, assets_texture->texture.id);
    return assets_texture;
}
