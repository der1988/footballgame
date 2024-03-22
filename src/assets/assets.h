#ifndef ASSETS_H
#define ASSETS_H

#include "../memory/memory.h"
#include "types.h"
#include <raylib.h>
#include <string.h>

// Texture Cache
typedef u32 AssetsTextureId;

typedef struct AssetsTexture {
    AssetsTextureId id;
    char name[30];
    Texture texture;
} AssetsTexture;

typedef struct AssetsTextureCache {
    b32 initialised;
    AssetsTexture *cache;
    u32 count;
} AssetsTextureCache;

AssetsTextureCache *assets_texture_cache_create(MemoryArenaHandle *memory_arena_handle);
void assets_texture_cache_destroy(AssetsTextureCache *handle);
AssetsTexture *assets_texture_load(AssetsTextureCache *handle, const char *filename);
AssetsTexture *assets_texture_get_by_id(AssetsTextureCache *handle, const u32 id);
AssetsTexture *assets_texture_get_by_name(AssetsTextureCache *handle, const char *filename);

// Sprite Frame Cache
typedef u32 AssetsSpriteFrameId;

typedef struct AssetsSpriteFrame {
    AssetsSpriteFrameId id;
    char name[30];
    Rectangle frame;
    b32 rotated;
    b32 trimmed;
    Rectangle sprite_source_size;
    Vector2 source_size;
    Vector2 pivot;
    u32 texture_id;
} AssetsSpriteFrame;

typedef struct AssetsSpriteFrameCache {
    b32 initialised;
    MemoryArenaHandle *memory_arena_handle;
    AssetsSpriteFrame *cache;
    u32 count;
} AssetsSpriteFrameCache;

AssetsSpriteFrameCache *assets_sprite_frame_cache_create(MemoryArenaHandle *memory_arena_handle);
void assets_sprite_frame_cache_destroy(AssetsSpriteFrameCache *handle);
i32 assets_sprite_frames_load_from_json(AssetsSpriteFrameCache *handle, const u32 texture_id,
                                        const char *filename);
AssetsSpriteFrame *assets_sprite_frame_get_by_name(AssetsSpriteFrameCache *handle,
                                                   const char *name);
AssetsSpriteFrame *assets_sprite_frame_get_by_id(AssetsSpriteFrameCache *handle,
                                                 const AssetsSpriteFrameId id);

// Texture Packer Helper
AssetsSpriteFrame *assets_texture_packer_load_from_json(const char *filename, const u32 texture_id,
                                                        const usize initial_sprite_frame_id,
                                                        usize *count);

// Animation Cache
typedef u32 AssetsAnimationId;

typedef struct AssetsAnimation {
    AssetsAnimationId id;
    char name[30];
    f32 delay;
    AssetsSpriteFrameId *frames;
    u16 frames_count;
} AssetsAnimation;

typedef struct AssetsAnimationCache {
    b32 initialised;
    MemoryArenaHandle *memory_arena_handle;
    AssetsAnimation *cache;
    u32 count;
} AssetsAnimationCache;

AssetsAnimationCache *assets_animation_cache_create(MemoryArenaHandle *memory_arena_handle);
void assets_animation_cache_destroy(AssetsAnimationCache *handle);
i32 assets_animations_load_from_json(AssetsAnimationCache *handle,
                                     AssetsSpriteFrameCache *sprite_frame_cache,
                                     const char *filename);
AssetsAnimation *assets_animation_get_by_name(AssetsAnimationCache *handle, const char *name);
AssetsAnimation *assets_animation_get_by_id(AssetsAnimationCache *handle,
                                            const AssetsAnimationId id);

#endif
