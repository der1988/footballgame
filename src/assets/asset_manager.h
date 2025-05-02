#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "memory.h"

#include "asset_animation.h"
#include "asset_shader.h"
#include "asset_sprite_frame.h"
#include "asset_texture.h"

struct assets {
	asset_texture_cache_t *texture_cache;
	asset_sprite_frame_cache_t *sprite_frame_cache;
	asset_animation_cache_t *animation_cache;
	asset_shader_cache_t *shader_cache;
};

struct assets asset_manager_load_everything(memory_arena_handle_t *memory_arena);
void asset_manager_destroy_everything(struct assets assets);

#endif // !ASSET_MANAGER_H
