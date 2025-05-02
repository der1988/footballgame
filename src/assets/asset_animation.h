#ifndef ASSET_ANIMATION_H
#define ASSET_ANIMATION_H

#include "types.h"
#include "asset_sprite_frame.h"

#define MAX_STRING_SIZE 50

// Animation Cache
typedef u32 assets_animation_id_t;

struct assets_animation {
	assets_animation_id_t id;
	char name[MAX_STRING_SIZE];
	f32 delay;
	assets_sprite_frame_id_t *frames;
	u16 frames_count;
};

typedef struct assets_animation_cache assets_animation_cache_t;

assets_animation_cache_t *assets_animation_cache_create(memory_arena_handle_t *memory_arena_handle);
void assets_animation_cache_destroy(assets_animation_cache_t *handle);
i32 assets_animations_load_from_json(assets_animation_cache_t *handle,
									 assets_sprite_frame_cache_t *sprite_frame_cache,
									 const char *filename);
struct assets_animation *assets_animation_get_by_name(assets_animation_cache_t *handle,
													  const char *name);
struct assets_animation *assets_animation_get_by_id(assets_animation_cache_t *handle,
													const assets_animation_id_t id);
assets_animation_id_t assets_animation_id_from_name(assets_animation_cache_t *handle,
													const char *name,
													const char *animation_name,
													memory_arena_handle_t *memory_arena_handle);

#endif // !ASSET_ANIMATION_H
