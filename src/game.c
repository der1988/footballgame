#include "game.h"
#include "assets/assets.h"
#include "components/components.h"
#include "config.h"
#include "entities/entities.h"
#include "football/football.h"
#include "input/input.h"
#include "layers/layers.h"
#include "memory/memory.h"
#include "raygui/raygui.h"
#include "systems/systems.h"
#include "tweakui/tweakui.h"
#include "types.h"
#include "utils.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>

#define PIXELS_PER_METER (1080.0f / 108.0f)

GameState *game_init(GameConfig game_config) {
    GLOGF_INFO(LS_GAME, "size of a component: %lu bytes", sizeof(Component));
    MemoryArenaHandle *memory_arena_lifetime_game =
        memory_arena_create("game", game_config.memory, Megabytes(1));

    GameState *game_state = memory_arena_push(memory_arena_lifetime_game, sizeof(GameState));
    game_state->config = game_config;
    game_state->config.players_count = 1;

    // pitch in reality is 105m long
    // pitch in game is 1080 pixels long
    // 1m is ~10 pixels in game
    // gravity is -9.81m/s^2 in reality, so -98.1 pixels/s^2 in game
    game_state->constants = (GameConstants){.fixed_timestep = 1 / 480.0,
                                            .render_debug = false,
                                            .input_stick_dead_zone = 0.05f,
                                            // Coefficient of restitution
                                            .ball_cr = 0.5f,
                                            .ball_cr_against_post = 0.7f,
                                            .pixels_per_meter = PIXELS_PER_METER,
                                            .air_density = 1.21f,
                                            .ball_cross_sectional_area = 0.038f,
                                            .ball_drag_coefficient = 0.25f,
                                            .gravity_force = {.y = -9.81f * PIXELS_PER_METER},
                                            .ball_friction_on_pitch = 0.5f,
                                            .ball_run_force = 20,
                                            .ball_control_force = 13,
                                            // This will translate to instant velocity, so 129km/h
                                            .ball_shot_force = 36 * PIXELS_PER_METER,
                                            .ball_shot_height = 0.35f,
                                            .ball_pass_force = 30 * PIXELS_PER_METER,
                                            .ball_long_pass_force = 26 * PIXELS_PER_METER,
                                            .ball_long_pass_height = 1.0f,
                                            .ball_after_touch_delay_time = 0.1f,
                                            .ball_after_touch_max_time = 0.75f,
                                            .ball_after_touch_force = 300.0f,
                                            // 36m/s seems fair, that would be 129km/h
                                            .ball_max_velocity = 50 * PIXELS_PER_METER,
                                            .ball_lookaheads = 4,
                                            .ball_lookahead_dts = 30,
                                            .player_movement_force = 25000,
                                            .player_movement_drag = 4166,
                                            .player_control_max_velocity = 6.6f * PIXELS_PER_METER,
                                            // Fastest player should reach ~10.5m/s, slowest 7.8m/s
                                            .player_run_max_velocity = 10.5f * PIXELS_PER_METER,
                                            .player_control_radius = 6,
                                            .player_control_height = 19,
                                            .player_run_control_max_angle = 45,
                                            .goalie_run_force = 10000,
                                            // Elite goalie has 150ms reaction time
                                            .goalie_dive_reaction_time = 0.150f,
                                            .goalie_dive_force = 8500,
                                            .goalie_dive_max_velocity = 5 * PIXELS_PER_METER,
                                            .shoot_aid_enabled = true,
                                            .shoot_aid_no_inertia = false,
                                            .kick_charge_reset_time = 1.5f};

    // Partition game memory
    game_state->memory_by_lifetime[MAL_GAME] = memory_arena_lifetime_game;
    game_state->memory_by_lifetime[MAL_LEVEL] =
        memory_arena_create("level", game_config.memory + Megabytes(1), Megabytes(1));
    game_state->memory_by_lifetime[MAL_UPDATE] =
        memory_arena_create("frame", game_config.memory + Megabytes(2), Megabytes(1));
    game_state->memory_by_lifetime[MAL_DRAW] =
        memory_arena_create("frame", game_config.memory + Megabytes(3), Megabytes(1));

    // Initialise entities repository
    game_state->entity_repository =
        entities_repository_init(game_state->memory_by_lifetime[MAL_GAME]);

    // Initialise caches
    game_state->texture_cache =
        assets_texture_cache_create(game_state->memory_by_lifetime[MAL_GAME]);
    game_state->sprite_frame_cache =
        assets_sprite_frame_cache_create(game_state->memory_by_lifetime[MAL_GAME]);
    game_state->animation_cache =
        assets_animation_cache_create(game_state->memory_by_lifetime[MAL_GAME]);

    // Load pitch
    AssetsTexture *texture = assets_texture_load(game_state->texture_cache, "res/pitch.png");
    assert(texture);
    game_state->pitch_texture_id = texture->id;

    // Load main spritesheet
    texture = assets_texture_load(game_state->texture_cache, "res/spritesheets/footballgame.png");
    assert(texture);
    i32 loaded = assets_sprite_frames_load_from_json(game_state->sprite_frame_cache, texture->id,
                                                     "res/spritesheets/footballgame.json");
    assert(loaded != -1);

    // Load other spritesheet
    texture = assets_texture_load(game_state->texture_cache, "res/spritesheets/other.png");
    assert(texture);
    loaded = assets_sprite_frames_load_from_json(game_state->sprite_frame_cache, texture->id,
                                                 "res/spritesheets/other.json");
    assert(loaded != -1);

    // Load Animations
    assets_animations_load_from_json(game_state->animation_cache, game_state->sprite_frame_cache,
                                     "res/animations/animations.json");

    game_state->tweakui = tweakui_create(game_state->memory_by_lifetime[MAL_GAME]);

    // Layers
    game_state->layer_debug = layers_debug_create(game_state);

#ifndef RUNNING_TEST
    game_state->config.players_count = 1;
    game_state->vars.pitch.rectangle =
        (Rectangle){.x = 33, .y = 77, .width = 640, .height = 360 * 3};

    Entity *player_entity = entities_player_create(
        (PlayerConfig){.position = {game_state->config.virtual_screen_width / 2.0f, 220.0f}},
        game_state);

    entities_player_create(
        (PlayerConfig){
            .is_goalie = true,
            .position = {game_state->config.virtual_screen_width / 2.0f - 100.0f, 220.0f}},
        game_state);

    Entity *ball_entity = entities_ball_create(
        (BallConfig){.position = {game_state->config.virtual_screen_width / 2.0f, 220.0f}},
        game_state);
    game_state->vars.ball_entity_id = ball_entity->id;

    entities_goal_create((GoalConfig){.position = GP_NORTH}, game_state);

    Entity *camera_entity = entities_camera_create(game_state);
    ComponentCamera *camera =
        &entities_component_get(game_state->entity_repository, CT_CAMERA, camera_entity->id)
             ->camera;
    ComponentTarget *camera_target =
        &entities_component_get(game_state->entity_repository, CT_TARGET, camera_entity->id)
             ->target;
    camera->camera = CAM_MAIN;
    camera_target->entity_id = ball_entity->id;
    game_state->vars.camera_entity_id = camera_entity->id;

    game_state->camera.zoom = 1;
    game_state->camera.rotation = 0;
    // game_state->camera.offset = (Vector2){(f32)game_state->config.screen_width / 2,
    //                                       (f32)game_state->config.screen_height / 2};
    game_state->virtual_camera.zoom = 1;
    game_state->virtual_camera.rotation = 0;
    game_state->virtual_camera.offset =
        (Vector2){(f32)game_state->config.virtual_screen_width / 2,
                  (f32)game_state->config.virtual_screen_height / 2};

    football_match_change_state(game_state, MS_PLAY);
    game_state->vars.p1 = (HumanPlayer){.player_entity_id = player_entity->id};
#endif

    game_state->render_texture = LoadRenderTexture((f32)game_state->config.virtual_screen_width,
                                                   (f32)game_state->config.virtual_screen_height);

    game_state->initialised = true;
    GLOG_INFO(LS_GAME, "game initialisation complete");

    return game_state;
}

void game_destroy(GameState *game_state) {
    assert(game_state->initialised);

    UnloadRenderTexture(game_state->render_texture);

    layers_debug_destroy(game_state->layer_debug);

    tweakui_destroy(game_state->tweakui);

    assets_animation_cache_destroy(game_state->animation_cache);
    assets_sprite_frame_cache_destroy(game_state->sprite_frame_cache);
    assets_texture_cache_destroy(game_state->texture_cache);

    entities_repository_destroy(game_state->entity_repository);

    memory_arena_destroy(game_state->memory_by_lifetime[MAL_GAME]);
    memory_arena_destroy(game_state->memory_by_lifetime[MAL_LEVEL]);
    memory_arena_destroy(game_state->memory_by_lifetime[MAL_UPDATE]);

    game_state->initialised = false;
    GLOG_INFO(LS_GAME, "game destruction complete");
}

void game_update(GameState *game_state, Input *input) {
    assert(game_state->initialised);

    // These should be reset every update
    memory_arena_reset(game_state->memory_by_lifetime[MAL_UPDATE]);
    layers_debug_reset(game_state->layer_debug);

    // Slow motion
    game_state->vars.slow_motion = input->buttons[IBT_BUTTON_TRIGGER_LEFT].down;
    if (game_state->vars.slow_motion) {
        game_state->time -= game_state->dt;
        game_state->dt = game_state->dt * 0.1;
        game_state->time += game_state->dt;
        game_state->vars.slow_motion = true;
    }

    systems_controls(game_state, input);
    systems_goalie(game_state);
    systems_player(game_state, input);
    systems_ball(game_state, input);
    systems_goal(game_state);
    systems_action(game_state);
    systems_camera(game_state);
}

void game_draw(GameState *game_state) {
    assert(game_state->initialised);

    memory_arena_reset(game_state->memory_by_lifetime[MAL_DRAW]);

    ComponentTransform *camera_transform = entities_component_get_typed(
        game_state->entity_repository, CT_TRANSFORM, game_state->vars.camera_entity_id);

    // Round virtual space coordinates, keep decimals into screen space coordinates
    f32 virtual_ratio =
        (f32)game_state->config.screen_width / (f32)game_state->config.virtual_screen_width;

    game_state->virtual_camera.target.x = (u64)camera_transform->position.x;
    game_state->camera.target.x =
        camera_transform->position.x - game_state->virtual_camera.target.x;
    game_state->camera.target.x *= virtual_ratio;

    game_state->virtual_camera.target.y = (u64)camera_transform->position.y;
    game_state->camera.target.y =
        camera_transform->position.y - game_state->virtual_camera.target.y;
    game_state->camera.target.y *= virtual_ratio;

    Texture2D pitch_texture =
        assets_texture_get_by_id(game_state->texture_cache, game_state->pitch_texture_id)->texture;

    BeginTextureMode(game_state->render_texture);
    ClearBackground((Color){.r = 42, .g = 99, .b = 1, .a = 255});

    BeginMode2D(game_state->virtual_camera);

    f32 stretched_width = pitch_texture.width * 1.0f;
    f32 stretched_height = pitch_texture.height * 1.0f;
    DrawTexturePro(
        pitch_texture,
        (Rectangle){.x = 0, .y = 0, .width = pitch_texture.width, .height = pitch_texture.height},
        (Rectangle){.x = 0, .y = 0, .width = stretched_width, .height = stretched_height},
        Vector2Zero(), 0, WHITE);

    systems_render(game_state);
    systems_render_debug(game_state);

    EndMode2D();

    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);

    // The target's height is flipped (in the source Rectangle), due to OpenGL reasons
    Rectangle source_rec = {0.0f, 0.0f, (float)game_state->render_texture.texture.width,
                            -(float)game_state->render_texture.texture.height};
    Rectangle dest_rec = {-virtual_ratio, -virtual_ratio,
                          game_state->config.screen_width + (virtual_ratio * 2),
                          game_state->config.screen_height + (virtual_ratio * 2)};

    Vector2 origin = {0.0f, 0.0f};

    BeginMode2D(game_state->camera);
    DrawTexturePro(game_state->render_texture.texture, source_rec, dest_rec, origin, 0.0f, WHITE);
    EndMode2D();

    layers_hud_draw(game_state);
    layers_debug_draw(game_state->layer_debug);

    // Tweak UI
    if (game_state->vars.is_tweakui_shown) {
        tweakui_draw(game_state->tweakui, game_state);
    }

    EndDrawing();
}
