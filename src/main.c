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

    double t = 0.0;
    double dt = 1 / 480.0;

    double currentTime = GetTime();

    //--------------------------------------------------------------------------------------

    // Main game loop;
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        double newTime = GetTime();
        double frameTime = newTime - currentTime;
        currentTime = newTime;

        PollInputEvents();

        // Handle Input
        Input input = input_handle(game_state);

        while (frameTime > 0.0) {
            float deltaTime = fminf(frameTime, dt);

            if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) {
                // Slow motion
                game_state->dt = deltaTime * 0.1f;
                game_state->time += game_state->dt;
            } else {
                game_state->time = t;
                game_state->dt = deltaTime;
            }

            // Update
            game_update(game_state, &input);

            frameTime -= deltaTime;
            t += deltaTime;
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
