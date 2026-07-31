#ifndef ASSET_H
#define ASSET_H

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

#endif // !ASSET_H
