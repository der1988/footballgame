#include "assets.h"
#include "game.h"
#include "memory.h"
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
    struct assets assets = assets_load_everything(main_memory_arena);

    struct game_config game_config = (struct game_config){
	.screen_width = GetScreenWidth(),
	.screen_height = GetScreenHeight(),
	.virtual_screen_width = virtual_screen_width,
	.virtual_screen_height = virtual_screen_height,
    };

    game_loop(game_config, assets, main_memory_arena);

    // De-Initialization
    //--------------------------------------------------------------------------------------

    assets_destroy_everything(assets);

    memory_arena_destroy(main_memory_arena);

    free(memory);

    CloseWindow();

    //--------------------------------------------------------------------------------------

    return 0;
}
