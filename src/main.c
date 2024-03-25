#include "assets/assets.h"
#include "entities/entities.h"
#include "game.h"
#include "input/input.h"
#include "memory/memory.h"
#include "types.h"
#include "utils/utils.h"
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>

int main() {
    // Initialization
    //--------------------------------------------------------------------------------------

    const float screen_width = 1280;
    const float screen_height = 960;

    InitWindow(screen_width, screen_height, "The Game");

    SetTargetFPS(60);

    void *memory = malloc(Megabytes(64));

    GameConfig game_config = (GameConfig){
        .screen_width = GetScreenWidth(), .screen_height = GetScreenHeight(), .memory = memory};

    GameState *game_state = game_init(game_config);

    //--------------------------------------------------------------------------------------

    // Main game loop;
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        game_state->time = GetTime();
        game_state->dt = GetFrameTime();

        // Handle Input
        Input input = input_handle(game_state);

        // Update
        game_update(game_state, &input);

        // Draw
        game_draw(game_state);
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------

    game_destroy(game_state);

    free(memory);

    CloseWindow();

    //--------------------------------------------------------------------------------------

    return 0;
}
