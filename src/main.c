#include "assets.h"
#include "game.h"
#include "memory.h"
#include "scenes.h"
#include "types.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>

int main()
{
    // Initialization
    //--------------------------------------------------------------------------------------

    const f32 screen_width = 1920;
    const f32 screen_height = 1080;
    const f32 virtual_screen_width = 640;
    const f32 virtual_screen_height = 360;

    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(screen_width, screen_height, "The Game");

    SetWindowMonitor(0);

    void *memory = malloc(Megabytes(64));
    memory_arena_handle_t *main_memory_arena = memory_arena_create("main", memory, Megabytes(64));

    // Load assets
    assets_texture_cache_t *texture_cache = assets_texture_cache_create(main_memory_arena);
    assets_sprite_frame_cache_t *sprite_frame_cache =
	assets_sprite_frame_cache_create(main_memory_arena);
    assets_animation_cache_t *animation_cache = assets_animation_cache_create(main_memory_arena);
    assets_shader_cache_t *shader_cache = assets_shader_cache_create(main_memory_arena);

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

    struct game_config game_config = (struct game_config){
	.screen_width = GetScreenWidth(),
	.screen_height = GetScreenHeight(),
	.virtual_screen_width = virtual_screen_width,
	.virtual_screen_height = virtual_screen_height,
    };

    struct scene_manager *scene_manager =
	scenes_manager_init(game_config, &assets, main_memory_arena);

    b32 window_should_close = false;

    while (!window_should_close) {
	window_should_close = scenes_manager_update(scene_manager);

	SwapScreenBuffer();
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------

    scenes_manager_destroy(scene_manager);

    assets_shader_cache_destroy(shader_cache);
    assets_animation_cache_destroy(animation_cache);
    assets_sprite_frame_cache_destroy(sprite_frame_cache);
    assets_texture_cache_destroy(texture_cache);

    memory_arena_destroy(main_memory_arena);

    free(memory);

    CloseWindow();

    //--------------------------------------------------------------------------------------

    return 0;
}
