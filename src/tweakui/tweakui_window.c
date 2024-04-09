#include "../utils/utils.h"
#include "game.h"
#include "memory.h"
#include "tweakui.h"
#include <assert.h>
#include <raylib.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui/raygui.h"

TweakUI *tweakui_create(MemoryArenaHandle *memory_arena_handle) {
    TweakUI *handle = memory_arena_push(memory_arena_handle, sizeof(TweakUI));
    handle->initialised = true;

    handle->window_size.x = 800;
    handle->window_size.y = 800;

    GLOG_INFO(LS_TWEAKUI, "tweakui initialised");

    return handle;
}
void tweakui_destroy(TweakUI *handle) {
    assert(handle->initialised);
    handle->initialised = false;
    GLOG_INFO(LS_TWEAKUI, "tweakui destroyed");
}

#define ROW_SIZE 20
#define MOVE_DOWN_ONE_ROW(bounds) bounds.y += ROW_SIZE

void tweakui_draw(TweakUI *handle, GameState *game_state) {
    assert(handle->initialised);

    Rectangle window_bounds = {
        .x = game_state->config.screen_width / 2.0f - handle->window_size.x / 2.0f,
        .y = game_state->config.screen_height / 2.0f - handle->window_size.y / 2.0f,
        .width = handle->window_size.x,
        .height = handle->window_size.y};
    GuiPanel(window_bounds, "test");

    Rectangle slider_bounds = {
        .x = window_bounds.x + 200, .y = window_bounds.y + 50, .width = 300, .height = 10};

    GuiSlider(slider_bounds, "Gravity force",
              TextFormat("%.2f", game_state->constants.gravity_force.y),
              &game_state->constants.gravity_force.y, -800.0f, 0.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Air density", TextFormat("%.2f", game_state->constants.air_density),
              &game_state->constants.air_density, 0, 10.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Ball CR", TextFormat("%.2f", game_state->constants.ball_cr),
              &game_state->constants.ball_cr, 0, 1.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Ball drag coefficient",
              TextFormat("%.2f", game_state->constants.ball_drag_coefficient),
              &game_state->constants.ball_drag_coefficient, 0, 1.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Ball friction on pitch",
              TextFormat("%.2f", game_state->constants.ball_friction_on_pitch),
              &game_state->constants.ball_friction_on_pitch, 0, 3.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Ball run force",
              TextFormat("%.2f", game_state->constants.ball_run_force),
              &game_state->constants.ball_run_force, 0, 200.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Ball control force",
              TextFormat("%.2f", game_state->constants.ball_control_force),
              &game_state->constants.ball_control_force, 0, 100.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Ball shot force",
              TextFormat("%.2f", game_state->constants.ball_shot_force),
              &game_state->constants.ball_shot_force, 0, 100000.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Ball shot height",
              TextFormat("%.2f", game_state->constants.ball_shot_height),
              &game_state->constants.ball_shot_height, 0, 1.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Ball after touch delay time",
              TextFormat("%.2f", game_state->constants.ball_after_touch_delay_time),
              &game_state->constants.ball_after_touch_delay_time, 0, 1.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Ball after touch max time",
              TextFormat("%.2f", game_state->constants.ball_after_touch_max_time),
              &game_state->constants.ball_after_touch_max_time, 0, 1.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Ball after touch force",
              TextFormat("%.2f", game_state->constants.ball_after_touch_force),
              &game_state->constants.ball_after_touch_force, 0, 100.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Player movement force",
              TextFormat("%.2f", game_state->constants.player_movement_force),
              &game_state->constants.player_movement_force, 0, 40000.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Player movement drag",
              TextFormat("%.2f", game_state->constants.player_movement_drag),
              &game_state->constants.player_movement_drag, 0, 40000.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Player movement parachute",
              TextFormat("%.2f", game_state->constants.player_movement_parachute),
              &game_state->constants.player_movement_parachute, 0, 40000.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Player control max velocity",
              TextFormat("%.2f", game_state->constants.player_control_max_velocity),
              &game_state->constants.player_control_max_velocity, 0, 150.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Player run max velocity",
              TextFormat("%.2f", game_state->constants.player_control_max_velocity),
              &game_state->constants.player_control_max_velocity, 0, 500.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Player control radius",
              TextFormat("%.2f", game_state->constants.player_control_radius),
              &game_state->constants.player_control_radius, 0, 100.0f);
}

// game_state->constants = (GameConstants){.input_stick_dead_zone = 0.05f,
//                                         // Coefficient of restitution
//                                         .ball_cr = 0.5f,
//                                         .ball_size = 5.0f,
//                                         .pixels_per_meter = PIXELS_PER_METER,
//                                         .air_density = 1.21f,
//                                         .ball_cross_sectional_area = 0.038f,
//                                         .ball_drag_coefficient = 0.25f,
//                                         .gravity_force = {.y = -9.81f * PIXELS_PER_METER},
//                                         .ball_friction_on_pitch = 1.3f,
//                                         .ball_run_force = 100.0f,
//                                         .ball_control_force = 40.0f,
//                                         .ball_shot_force = 50000.0f,
//                                         .ball_shot_height = 0.5f,
//                                         .ball_after_touch_delay_time = 0.1f,
//                                         .ball_after_touch_max_time = 0.5f,
//                                         .ball_after_touch_force = 15.0f,
//                                         .player_movement_force = 22000.0f,
//                                         .player_movement_drag = 12500.0f,
//                                         .player_movement_parachute = 22500.0f,
//                                         .player_control_max_velocity = 150.0f,
//                                         .player_run_max_velocity = 250.0f,
//                                         .player_control_radius = 17.5f};
