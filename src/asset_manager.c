#include "macro.h"
#include "asset_manager.h"
#include "asset.h"
#include "memory.h"

struct assets asset_manager_load_everything(memory_arena_handle_t *memory_arena)
{
	// Load assets
	asset_texture_cache_t *texture_cache = asset_texture_cache_create(memory_arena);
	asset_sprite_frame_cache_t *sprite_frame_cache = asset_sprite_frame_cache_create(memory_arena);
	asset_animation_cache_t *animation_cache = asset_animation_cache_create(memory_arena);
	asset_shader_cache_t *shader_cache = asset_shader_cache_create(memory_arena);

	struct assets assets = {.texture_cache = texture_cache,
							.sprite_frame_cache = sprite_frame_cache,
							.animation_cache = animation_cache,
							.shader_cache = shader_cache};

	// Load pitch texture
	struct asset_texture *texture = asset_texture_load(texture_cache, "res/pitch.png");
	ASSERT(texture);

	// Player spritesheet
	texture = asset_texture_load(texture_cache, "res/spritesheets/sprites.png");
	ASSERT(texture);
	i32 loaded = asset_sprite_frames_load_from_json(
		sprite_frame_cache, texture->id, "res/spritesheets/sprites.json");
	ASSERT(loaded != -1);

	// Animations
	asset_animations_load_from_json(
		animation_cache, sprite_frame_cache, "res/animations/animations.json");

	// Shaders
#ifdef __EMSCRIPTEN__
	asset_shader_load(shader_cache, "shader_palette", "res/shaders/web_base.vert", "res/shaders/web_palette.frag");
#else
	asset_shader_load(
		shader_cache, "shader_palette", "res/shaders/base.vert", "res/shaders/palette.frag");
#endif

	return assets;
}

void asset_manager_destroy_everything(struct assets assets)
{
	asset_shader_cache_destroy(assets.shader_cache);
	asset_animation_cache_destroy(assets.animation_cache);
	asset_sprite_frame_cache_destroy(assets.sprite_frame_cache);
	asset_texture_cache_destroy(assets.texture_cache);
}
