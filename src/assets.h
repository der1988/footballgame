#ifndef ASSETS_H
#define ASSETS_H

#include "memory.h"
#include "types.h"
#include <raylib.h>

#define MaxStringSize 50

// Texture Cache
typedef u32 AssetsTextureId;

typedef struct AssetsTextureCache AssetsTextureCache;

struct AssetsTexture
{
    AssetsTextureId id;
    char name[MaxStringSize];
    struct Texture texture;
};

struct AssetsTextureCache *assets_texture_cache_create(MemoryArenaHandle *memory_arena_handle);
void assets_texture_cache_destroy(struct AssetsTextureCache *handle);
struct AssetsTexture *assets_texture_load(struct AssetsTextureCache *handle, const char *filename);
struct AssetsTexture *assets_texture_get_by_id(struct AssetsTextureCache *handle, const u32 id);
struct AssetsTexture *assets_texture_get_by_name(struct AssetsTextureCache *handle,
						 const char *filename);

// Sprite Frame Cache
typedef u32 AssetsSpriteFrameId;

typedef struct AssetsSpriteFrameCache AssetsSpriteFrameCache;

struct AssetsSpriteFrame
{
    AssetsSpriteFrameId id;
    char name[MaxStringSize];
    Rectangle frame;
    b32 rotated;
    b32 trimmed;
    Rectangle sprite_source_size;
    v2 source_size;
    v2 pivot;
    u32 texture_id;
};

AssetsSpriteFrameCache *assets_sprite_frame_cache_create(MemoryArenaHandle *memory_arena_handle);
void assets_sprite_frame_cache_destroy(AssetsSpriteFrameCache *handle);
i32 assets_sprite_frames_load_from_json(AssetsSpriteFrameCache *handle, const u32 texture_id,
					const char *filename);
struct AssetsSpriteFrame *assets_sprite_frame_get_by_name(AssetsSpriteFrameCache *handle,
							  const char *name);
struct AssetsSpriteFrame *assets_sprite_frame_get_by_id(AssetsSpriteFrameCache *handle,
							const AssetsSpriteFrameId id);

// Texture Packer Helper
struct AssetsSpriteFrame *assets_texture_packer_load_from_json(const char *filename,
							       const u32 texture_id,
							       const usize initial_sprite_frame_id,
							       usize *count);

// Animation Cache
typedef u32 AssetsAnimationId;

struct AssetsAnimation
{
    AssetsAnimationId id;
    char name[MaxStringSize];
    f32 delay;
    AssetsSpriteFrameId *frames;
    u16 frames_count;
};

typedef struct AssetsAnimationCache AssetsAnimationCache;

AssetsAnimationCache *assets_animation_cache_create(MemoryArenaHandle *memory_arena_handle);
void assets_animation_cache_destroy(AssetsAnimationCache *handle);
i32 assets_animations_load_from_json(AssetsAnimationCache *handle,
				     AssetsSpriteFrameCache *sprite_frame_cache,
				     const char *filename);
struct AssetsAnimation *assets_animation_get_by_name(AssetsAnimationCache *handle,
						     const char *name);
struct AssetsAnimation *assets_animation_get_by_id(AssetsAnimationCache *handle,
						   const AssetsAnimationId id);

#endif
