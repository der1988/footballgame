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

    InitWindow(screen_width, screen_height, "The Game");

    void *memory = malloc(Megabytes(64));

    GameConfig game_config = (GameConfig){
        .screen_width = GetScreenWidth(), .screen_height = GetScreenHeight(), .memory = memory};

    GameState *game_state = game_init(game_config);

    const double dt = 1 / 480.0;

    double current_time = GetTime();
    double accumulator = 0.0;

    //--------------------------------------------------------------------------------------

    // Main game loop;
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        double new_time = GetTime();
        double frame_time = new_time - current_time;
        current_time = new_time;

        accumulator += frame_time;
        while (accumulator >= dt) {

            PollInputEvents();

            // Handle Input
            Input input = input_handle(game_state);

            b32 slow_motion = IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1);

            game_state->dt = dt * (slow_motion ? 0.1f : 1);

            // Update
            game_update(game_state, &input);

            accumulator -= game_state->dt;
            game_state->time += game_state->dt;
        }

        // Draw
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
