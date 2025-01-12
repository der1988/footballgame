#include "assets.h"
#include "memory.h"
#include <assert.h>

struct assets assets_load_everything(memory_arena_handle_t *memory_arena)
{
	// Load assets
	assets_texture_cache_t *texture_cache = assets_texture_cache_create(memory_arena);
	assets_sprite_frame_cache_t *sprite_frame_cache =
		assets_sprite_frame_cache_create(memory_arena);
	assets_animation_cache_t *animation_cache = assets_animation_cache_create(memory_arena);
	assets_shader_cache_t *shader_cache = assets_shader_cache_create(memory_arena);

	struct assets assets = {.texture_cache = texture_cache,
							.sprite_frame_cache = sprite_frame_cache,
							.animation_cache = animation_cache,
							.shader_cache = shader_cache};

	// Load pitch texture
	struct assets_texture *texture = assets_texture_load(texture_cache, "res/pitch.png");
	assert(texture);

	// Player spritesheet
	texture = assets_texture_load(texture_cache, "res/spritesheets/player.png");
	assert(texture);
	i32 loaded = assets_sprite_frames_load_from_json(
		sprite_frame_cache, texture->id, "res/spritesheets/player.json");
	assert(loaded != -1);

	// Goalkeeper spritesheet
	texture = assets_texture_load(texture_cache, "res/spritesheets/goalkeeper.png");
	assert(texture);
	loaded = assets_sprite_frames_load_from_json(
		sprite_frame_cache, texture->id, "res/spritesheets/goalkeeper.json");
	assert(loaded != -1);

	// Ball spritesheet
	texture = assets_texture_load(texture_cache, "res/spritesheets/ball.png");
	assert(texture);
	loaded = assets_sprite_frames_load_from_json(
		sprite_frame_cache, texture->id, "res/spritesheets/ball.json");
	assert(loaded != -1);

	// Other spritesheets
	texture = assets_texture_load(texture_cache, "res/spritesheets/other.png");
	assert(texture);
	loaded = assets_sprite_frames_load_from_json(
		sprite_frame_cache, texture->id, "res/spritesheets/other.json");
	assert(loaded != -1);

	// Animations
	assets_animations_load_from_json(
		animation_cache, sprite_frame_cache, "res/animations/animations.json");

	// Shaders
	assets_shader_load(shader_cache, "shader_palette", NULL, "res/shaders/palette.frag");

	return assets;
}

void assets_destroy_everything(struct assets assets)
{
	assets_shader_cache_destroy(assets.shader_cache);
	assets_animation_cache_destroy(assets.animation_cache);
	assets_sprite_frame_cache_destroy(assets.sprite_frame_cache);
	assets_texture_cache_destroy(assets.texture_cache);
}
