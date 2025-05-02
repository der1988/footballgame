#ifndef ASSET_ANIMATION_H
#define ASSET_ANIMATION_H

#include "types.h"
#include "as_sprite_frame.h"

// Entity name is 20 chars, animation name should be 30 chars at most
#define ANIMATION_NAME_MAX_SIZE 80
// Entity name is 20 chars, animation prefix should be 30 chars at most
#define ANIMATION_FRAME_MAX_SIZE 80

// Animation Cache
typedef u32 asset_animation_id_t;

struct asset_animation {
	asset_animation_id_t id;
	char name[ANIMATION_NAME_MAX_SIZE];
	f32 delay;
	asset_sprite_frame_id_t *frames;
	u16 frames_count;
};

typedef struct asset_animation_cache asset_animation_cache_t;

asset_animation_cache_t *asset_animation_cache_create(memory_arena_handle_t *memory_arena_handle);
void asset_animation_cache_destroy(asset_animation_cache_t *handle);
i32 asset_animations_load_from_json(asset_animation_cache_t *handle,
									asset_sprite_frame_cache_t *sprite_frame_cache,
									const char *filename);
struct asset_animation *asset_animation_get_by_name(asset_animation_cache_t *handle,
													const char *name);
struct asset_animation *asset_animation_get_by_id(asset_animation_cache_t *handle,
												  const asset_animation_id_t id);
asset_animation_id_t asset_animation_id_from_name(asset_animation_cache_t *handle,
												  const char *name,
												  const char *animation_name,
												  memory_arena_handle_t *memory_arena_handle);

#endif // !ASSET_ANIMATION_H
