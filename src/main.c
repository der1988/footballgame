#include "game.h"
#include "input/input.h"
#include "types.h"
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>

int main() {
    // Initialization
    //--------------------------------------------------------------------------------------

    const float screen_width = 1920;
    const float screen_height = 1080;
    const float virtual_screen_width = 640;
    const float virtual_screen_height = 360;

    InitWindow(screen_width, screen_height, "The Game");

    void *memory = malloc(Megabytes(64));

    GameConfig game_config = (GameConfig){.screen_width = GetScreenWidth(),
                                          .screen_height = GetScreenHeight(),
                                          .virtual_screen_width = virtual_screen_width,
                                          .virtual_screen_height = virtual_screen_height,
                                          .memory = memory};

    GameState *game_state = game_init(game_config);

    // Fixed timestep
    const double dt = 1 / 480.0;

    double current_time = GetTime();
    double accumulator = 0.0;

    //--------------------------------------------------------------------------------------

    while (!WindowShouldClose()) {
        double new_time = GetTime();
        double frame_time = new_time - current_time;
        current_time = new_time;

        accumulator += frame_time;
        while (accumulator > 0) {

            PollInputEvents();

            Input input = input_handle(game_state);

            game_state->dt = dt;

            game_update(game_state, &input);

            accumulator -= dt;
            game_state->time += dt;
        }

        game_draw(game_state);

        SwapScreenBuffer();
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------

    game_destroy(game_state);

    free(memory);

    CloseWindow();

    //--------------------------------------------------------------------------------------

    return 0;
}
