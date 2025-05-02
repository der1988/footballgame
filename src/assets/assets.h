#ifndef ASSETS_H
#define ASSETS_H

#include "as_animation.h"
#include "as_shader.h"
#include "as_sprite_frame.h"
#include "as_texture.h"

struct assets {
	asset_texture_cache_t *texture_cache;
	asset_sprite_frame_cache_t *sprite_frame_cache;
	asset_animation_cache_t *animation_cache;
	asset_shader_cache_t *shader_cache;
};

#endif // !ASSETS_H
