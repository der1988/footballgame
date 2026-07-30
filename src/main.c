#include "game/game.h"
#include "assets/as_manager.h"
#include <stdlib.h>

#define TOTAL_MEMORY_MBS 2000

int main()
{
	// Initialization
	//--------------------------------------------------------------------------------------

	const f32 screen_width = 1920;
	const f32 screen_height = 1080;
	const f32 virtual_screen_width = 640;
	const f32 virtual_screen_height = 360;

	// SetConfigFlags(FLAG_VSYNC_HINT);
	InitWindow(screen_width, screen_height, "The Game");

	SetWindowMonitor(0);

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
