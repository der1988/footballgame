#ifndef ASSET_SPRITE_FRAME_H
#define ASSET_SPRITE_FRAME_H

#include "types.h"
#include "memory.h"

#define SPRITE_FRAME_NAME_MAX_SIZE 50

// Sprite Frame Cache
typedef u32 asset_sprite_frame_id_t;

typedef struct asset_sprite_frame_cache asset_sprite_frame_cache_t;

struct asset_sprite_frame {
	asset_sprite_frame_id_t id;
	char name[SPRITE_FRAME_NAME_MAX_SIZE];
	rect_t frame;
	b32 rotated;
	b32 trimmed;
	rect_t sprite_source_size;
	v2 source_size;
	v2 pivot;
	u32 texture_id;
};

asset_sprite_frame_cache_t *
asset_sprite_frame_cache_create(memory_arena_handle_t *memory_arena_handle);
void asset_sprite_frame_cache_destroy(asset_sprite_frame_cache_t *handle);
i32 asset_sprite_frames_load_from_json(asset_sprite_frame_cache_t *handle,
									   const u32 texture_id,
									   const char *filename);
struct asset_sprite_frame *asset_sprite_frame_get_by_name(asset_sprite_frame_cache_t *handle,
														  const char *name);
struct asset_sprite_frame *asset_sprite_frame_get_by_id(asset_sprite_frame_cache_t *handle,
														const asset_sprite_frame_id_t id);

#endif // !ASSET_SPRITE_FRAME_H
