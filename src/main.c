#include "game.h"
#include "input/input.h"
#include "layers.h"
#include "types.h"
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Initialization
    //--------------------------------------------------------------------------------------

    const f32 screen_width = 1920;
    const f32 screen_height = 1080;
    const f32 virtual_screen_width = 640;
    const f32 virtual_screen_height = 360;

    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(screen_width, screen_height, "The Game");

    void *memory = malloc(Megabytes(64));

    GameConfig game_config = (GameConfig){.screen_width = GetScreenWidth(),
                                          .screen_height = GetScreenHeight(),
                                          .virtual_screen_width = virtual_screen_width,
                                          .virtual_screen_height = virtual_screen_height,
                                          .memory = memory};

    GameState *game_state = game_init(game_config);

    f64 current_time = GetTime();
    f64 accumulator = 0.0;
    f64 dt = game_state->constants.fixed_timestep;
    b32 window_should_close = false;

    while (!window_should_close) {
        if (game_state->vars.is_tweakui_shown) {
            PollInputEvents();
            window_should_close = WindowShouldClose();
            if (IsKeyReleased(KEY_P)) {
                game_state->vars.is_tweakui_shown = false;
            }
        } else {
            // Update game
            f64 new_time = GetTime();
            f64 frame_time = new_time - current_time;
            f64 total_update_time = 0;
            current_time = new_time;

            u32 physics_iterations = 0;
            accumulator += frame_time;
            while (accumulator > 0) {
                PollInputEvents();
                window_should_close |= WindowShouldClose();
                if (IsKeyReleased(KEY_P)) {
                    game_state->vars.is_tweakui_shown = true;
                }
                Input input = input_handle(game_state);

                game_state->dt = dt;

                f64 update_time = GetTime();
                game_update(game_state, &input);
                total_update_time += GetTime() - update_time;

                accumulator -= dt;
                game_state->time += dt;

                physics_iterations++;
            }

            layers_debug_queue_draw_text(
                game_state->layer_debug,
                TextFormat("Update time: %.6f", total_update_time / physics_iterations));
            layers_debug_queue_draw_text(game_state->layer_debug,
                                         TextFormat("Physics iter/frame: %d", physics_iterations));
            layers_debug_queue_draw_text(game_state->layer_debug,
                                         TextFormat("FPS: %.2f", 1 / frame_time));
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
