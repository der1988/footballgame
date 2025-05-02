#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "memory.h"

#include "asset_animation.h"
#include "asset_shader.h"
#include "asset_sprite_frame.h"
#include "asset_texture.h"

struct assets {
	assets_texture_cache_t *texture_cache;
	assets_sprite_frame_cache_t *sprite_frame_cache;
	assets_animation_cache_t *animation_cache;
	assets_shader_cache_t *shader_cache;
};

struct assets assets_load_everything(memory_arena_handle_t *memory_arena);
void assets_destroy_everything(struct assets assets);

#endif // !ASSET_MANAGER_H
