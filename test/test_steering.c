#include "game.h"
#include "types.h"
#include "unity/unity.h"
#include "unity/unity_internals.h"
#include <assert.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct GameState *g_game_state = NULL;

void setUp(void) {
    int width = 800;
    int height = 600;
    InitWindow(width, height, "Test Steering");

    SetTargetFPS(60);

    void *memory = malloc(Megabytes(10));

    struct GameConfig game_config =
        (struct GameConfig){.screen_width = width, .screen_height = height, .memory = memory};

    g_game_state = game_init(game_config);
}

void tearDown(void) {
    void *memory = g_game_state->config.memory;

    game_destroy(g_game_state);

    free(memory);

    CloseWindow();
}

void test_steering() {
    // Main game loop;
    // while (!WindowShouldClose()) // Detect window close button or ESC key
    // {
    //     g_game_state->time = GetTime();
    //     g_game_state->dt = GetFrameTime();
    //
    //     // Handle Input
    //     Input input = input_handle(g_game_state);
    //
    //     // Update
    //     game_update(g_game_state, &input);
    //
    //     // Draw
    //     game_draw(g_game_state);
    // }
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_steering);

    return UNITY_END();
}
