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

    const float screen_width = 1920;
    const float screen_height = 1080;

    InitWindow(screen_width, screen_height, "The Game");

    void *memory = malloc(Megabytes(64));

    GameConfig game_config = (GameConfig){
        .screen_width = GetScreenWidth(), .screen_height = GetScreenHeight(), .memory = memory};

    GameState *game_state = game_init(game_config);

    // Custom timing variables
    double previousTime = GetTime(); // Previous time measure
    double currentTime = 0.0;        // Current time measure
    double updateDrawTime = 0.0;     // Update + Draw time
    double waitTime = 0.0;           // Wait time (if target fps required)
    float deltaTime = 0.0f;          // Frame time (Update + Draw + Wait time)

    float timeCounter = 0.0f; // Accumulative time counter (seconds)
    float position = 0.0f;    // Circle position
    bool pause = false;       // Pause control flag

    int targetFPS = 60; // Our initial target fps

    //--------------------------------------------------------------------------------------

    // Main game loop;
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        PollInputEvents();

        if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) {
            // Slow motion
            game_state->dt = deltaTime * 0.1f;
            game_state->time += game_state->dt;
        } else {
            game_state->time = GetTime();
            game_state->dt = deltaTime;
        }

        // Handle Input
        Input input = input_handle(game_state);

        // Update
        game_update(game_state, &input);

        // Draw
        game_draw(game_state);

        SwapScreenBuffer(); // Flip the back buffer to screen (front buffer)

        currentTime = GetTime();
        updateDrawTime = currentTime - previousTime;

        if (targetFPS > 0) // We want a fixed frame rate
        {
            waitTime = (1.0f / (float)targetFPS) - updateDrawTime;
            if (waitTime > 0.0) {
                WaitTime((float)waitTime);
                currentTime = GetTime();
                deltaTime = (float)(currentTime - previousTime);
            }
        } else
            deltaTime = (float)updateDrawTime; // Framerate could be variable

        previousTime = currentTime;
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------

    game_destroy(game_state);

    free(memory);

    CloseWindow();

    //--------------------------------------------------------------------------------------

    return 0;
}
