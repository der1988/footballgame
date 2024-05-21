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
    handle->window_size.y = 700;

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

    Rectangle window_bounds = {.x = game_state->config.screen_width / 2.0f -
                                    handle->window_size.x / 2.0f,
                               .y = 10,
                               .width = handle->window_size.x,
                               .height = handle->window_size.y};
    GuiPanel(window_bounds, "Tweak UI");

    Rectangle slider_bounds = {
        .x = window_bounds.x + 200, .y = window_bounds.y + 50, .width = 500, .height = 10};

    GuiToggle((Rectangle){.x = slider_bounds.x, .y = slider_bounds.y, .width = 100, .height = 15},
              "Render Debug", (bool *)&game_state->constants.render_debug);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiToggle((Rectangle){.x = slider_bounds.x, .y = slider_bounds.y, .width = 100, .height = 15},
              "AI Enabled", (bool *)&game_state->constants.ai_enabled);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiToggle((Rectangle){.x = slider_bounds.x, .y = slider_bounds.y, .width = 100, .height = 15},
              "Shoot Aid Enabled", (bool *)&game_state->constants.shoot_aid);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Gravity force",
              TextFormat("%.2f", game_state->constants.gravity_force.y),
              &game_state->constants.gravity_force.y, -800.0f, 0.0f);
    // MOVE_DOWN_ONE_ROW(slider_bounds);
    // GuiSlider(slider_bounds, "Air density", TextFormat("%.2f",
    // game_state->constants.air_density),
    //           &game_state->constants.air_density, 0, 10.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Ball CR", TextFormat("%.2f", game_state->constants.ball_cr),
              &game_state->constants.ball_cr, 0, 1.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Ball CR against post",
              TextFormat("%.2f", game_state->constants.ball_cr_against_post),
              &game_state->constants.ball_cr_against_post, 0, 1.0f);
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
              &game_state->constants.ball_shot_force, 0, 1000.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Ball shot height",
              TextFormat("%.2f", game_state->constants.ball_shot_height),
              &game_state->constants.ball_shot_height, 0, 1.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Ball long pass force",
              TextFormat("%.2f", game_state->constants.ball_long_pass_force),
              &game_state->constants.ball_long_pass_force, 0, 1000.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Ball long pass height",
              TextFormat("%.2f", game_state->constants.ball_long_pass_height),
              &game_state->constants.ball_long_pass_height, 0, 1.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Ball short pass force",
              TextFormat("%.2f", game_state->constants.ball_pass_force),
              &game_state->constants.ball_pass_force, 0, 1000.0f);
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
              &game_state->constants.ball_after_touch_force, 0, 1000.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Player movement force",
              TextFormat("%.2f", game_state->constants.player_movement_force),
              &game_state->constants.player_movement_force, 0, 40000.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Player movement drag",
              TextFormat("%.2f", game_state->constants.player_movement_drag),
              &game_state->constants.player_movement_drag, 0, 40000.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Player control max velocity",
              TextFormat("%.2f", game_state->constants.player_control_max_velocity),
              &game_state->constants.player_control_max_velocity, 0, 500.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Player run max velocity",
              TextFormat("%.2f", game_state->constants.player_run_max_velocity),
              &game_state->constants.player_run_max_velocity, 0, 500.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Player control radius",
              TextFormat("%.2f", game_state->constants.player_control_radius),
              &game_state->constants.player_control_radius, 0, 100.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Player control height",
              TextFormat("%.2f", game_state->constants.player_control_height),
              &game_state->constants.player_control_height, 0, 30.0f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Player control run max angle",
              TextFormat("%.2f", game_state->constants.player_run_control_max_angle),
              &game_state->constants.player_run_control_max_angle, 0, 60);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Goalie reaction time",
              TextFormat("%.2f", game_state->constants.goalie_dive_reaction_time),
              &game_state->constants.goalie_dive_reaction_time, 0, 0.4f);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Goalie run force",
              TextFormat("%.2f", game_state->constants.goalie_run_force),
              &game_state->constants.goalie_run_force, 5000, 20000);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Goalie dive force",
              TextFormat("%.2f", game_state->constants.goalie_dive_force),
              &game_state->constants.goalie_dive_force, 5000, 20000);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Goalie dive max velocity",
              TextFormat("%.2f", game_state->constants.goalie_dive_max_velocity),
              &game_state->constants.goalie_dive_max_velocity, 0, 300);
    MOVE_DOWN_ONE_ROW(slider_bounds);
    GuiSlider(slider_bounds, "Kick charge reset time",
              TextFormat("%.2f", game_state->constants.kick_charge_reset_time),
              &game_state->constants.kick_charge_reset_time, 0, 2.0f);
}
