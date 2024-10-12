#ifndef ASSETS_INTERNAL_H
#define ASSETS_INTERNAL_H

#include "memory.h"
#include "types.h"

struct AssetsTextureCache
{
    b32 initialised;
    struct AssetsTexture *cache;
    u32 count;
};

struct AssetsSpriteFrameCache
{
    b32 initialised;
    MemoryArenaHandle *memory_arena_handle;
    struct AssetsSpriteFrame *cache;
    u32 count;
};

struct AssetsAnimationCache
{
    b32 initialised;
    MemoryArenaHandle *memory_arena_handle;
    struct AssetsAnimation *cache;
    u32 count;
};

#endif
