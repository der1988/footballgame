#include "game.h"
#include "asset_manager.h"
#include <stdlib.h>

#ifdef __EMSCRIPTEN__
#define TOTAL_MEMORY_MBS 64
#else
#define TOTAL_MEMORY_MBS 2000
#endif

int main()
{
	// Initialization
	//--------------------------------------------------------------------------------------

#ifdef __EMSCRIPTEN__
	const f32 screen_width = 1280;
	const f32 screen_height = 720;
#else
	const f32 screen_width = 1920;
	const f32 screen_height = 1080;
#endif
	const f32 virtual_screen_width = 640;
	const f32 virtual_screen_height = 360;

	// SetConfigFlags(FLAG_VSYNC_HINT);
	InitWindow(screen_width, screen_height, "The Game");
#ifdef __EMSCRIPTEN__
	SetExitKey(KEY_NULL);
#endif

#ifndef __EMSCRIPTEN__
	SetWindowMonitor(0);
#endif

	void *memory = malloc(Megabytes(TOTAL_MEMORY_MBS));
	memory_arena_handle_t *main_memory_arena =
		memory_arena_create_default("main", memory, Megabytes(TOTAL_MEMORY_MBS));

	// Load assets
	struct assets assets = asset_manager_load_everything(main_memory_arena);

	struct game_config game_config = (struct game_config){
		.screen_width = GetScreenWidth(),
		.screen_height = GetScreenHeight(),
		.virtual_screen_width = virtual_screen_width,
		.virtual_screen_height = virtual_screen_height,
		.gameplay_mode = GPM_MATCH,
	};

	game_loop(game_config, assets, main_memory_arena);

	// De-Initialization
	//--------------------------------------------------------------------------------------

	asset_manager_destroy_everything(assets);

	memory_arena_destroy(main_memory_arena);

	free(memory);

	CloseWindow();

	//--------------------------------------------------------------------------------------

	return 0;
}
