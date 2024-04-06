#include "game.h"
#include "assets/assets.h"
#include "components/components.h"
#include "entities/entities.h"
#include "input/input.h"
#include "memory/memory.h"
#include "systems/systems.h"
#include "types.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>

GameState *game_init(GameConfig game_config) {
    MemoryArenaHandle *memory_arena_lifetime_game =
        memory_arena_create("game", game_config.memory, Megabytes(1));

    GameState *game_state = memory_arena_push(memory_arena_lifetime_game, sizeof(GameState));
    game_state->config = game_config;
    game_state->config.players_count = 1;

    game_state->constants = (GameConstants){.input_stick_dead_zone = 0.05f,
                                            // Coefficient of restitution
                                            .ball_cr = 0.5f,
                                            .ball_size = 5.0f,
                                            .gravity_force = {.y = -9.81f},
                                            .ball_drag_in_air = 0.6f,
                                            .ball_drag_on_pitch = 1.5f,
                                            .ball_run_force = 175.0f,
                                            .ball_control_force = 20.0f,
                                            .ball_shot_force = 1000.0f,
                                            .ball_shot_height = 0.3f,
                                            .ball_after_touch_delay_time = 0.1f,
                                            .ball_after_touch_max_time = 0.5f,
                                            .ball_after_touch_force = 15.0f,
                                            .player_movement_force = 1500.0f,
                                            .player_movement_drag = 1000.0f,
                                            .player_control_max_velocity = 150.0f,
                                            .player_run_max_velocity = 250.0f};

    // Partition game memory
    game_state->memory_by_lifetime[MAL_GAME] = memory_arena_lifetime_game;
    game_state->memory_by_lifetime[MAL_LEVEL] =
        memory_arena_create("level", game_config.memory + Megabytes(1), Megabytes(1));
    game_state->memory_by_lifetime[MAL_FRAME] =
        memory_arena_create("frame", game_config.memory + Megabytes(2), Megabytes(1));

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

    // Load caches
    AssetsTexture *texture = assets_texture_load(game_state->texture_cache, "res/pitch.png");
    assert(texture);
    game_state->pitch_texture_id = texture->id;
    texture = assets_texture_load(game_state->texture_cache, "res/spritesheets/footballgame.png");
    assert(texture);
    i32 loaded = assets_sprite_frames_load_from_json(game_state->sprite_frame_cache, texture->id,
                                                     "res/spritesheets/footballgame.json");
    assert(loaded != -1);
    assets_animations_load_from_json(game_state->animation_cache, game_state->sprite_frame_cache,
                                     "res/animations/animations.json");

#ifndef RUNNING_TEST
    game_state->config.players_count = 1;

    Entity *player_entity = entities_player_create(game_state);
    ComponentTransform *player_transform =
        &entities_component_get(game_state->entity_repository, CT_TRANSFORM, player_entity->id)
             ->transform;

    Entity *ball_entity = entities_ball_create(game_state);
    game_state->vars.ball_entity_id = ball_entity->id;
    Component *ball_transform =
        entities_component_get(game_state->entity_repository, CT_TRANSFORM, ball_entity->id);
    ball_transform->transform.position = (Vector2){game_state->config.screen_width / 2.0f, 20.0f};

    Entity *camera = entities_camera_create(game_state);
    ComponentCameraProps *camera_props =
        &entities_component_get(game_state->entity_repository, CT_CAMERA_PROPS, camera->id)
             ->camera_props;
    ComponentTarget *camera_target =
        &entities_component_get(game_state->entity_repository, CT_TARGET, camera->id)->target;
    camera_props->camera = CAM_MAIN;
    camera_target->entity_id = ball_entity->id;

    game_state->render_texture = LoadRenderTexture((f32)game_state->config.screen_width,
                                                   (f32)game_state->config.screen_height);
    game_state->camera.zoom = 1;
    game_state->camera.rotation = 0;
    game_state->camera.offset = (Vector2){(f32)game_state->config.screen_width / 2,
                                          (f32)game_state->config.screen_height / 2};
#endif

    game_state->initialised = true;

    return game_state;
}

void game_destroy(GameState *game_state) {
    assert(game_state->initialised);

    UnloadRenderTexture(game_state->render_texture);

    assets_animation_cache_destroy(game_state->animation_cache);
    assets_sprite_frame_cache_destroy(game_state->sprite_frame_cache);
    assets_texture_cache_destroy(game_state->texture_cache);

    entities_repository_destroy(game_state->entity_repository);

    memory_arena_destroy(game_state->memory_by_lifetime[MAL_GAME]);
    memory_arena_destroy(game_state->memory_by_lifetime[MAL_LEVEL]);
    memory_arena_destroy(game_state->memory_by_lifetime[MAL_FRAME]);

    game_state->initialised = false;
}

void game_update(GameState *game_state, Input *input) {
    assert(game_state->initialised);

    // This memory arena should be reset every frame
    memory_arena_reset(game_state->memory_by_lifetime[MAL_FRAME]);

    systems_player(game_state, input);
    systems_ball(game_state, input);
    systems_action(game_state);
    systems_camera(game_state);
}

void game_draw(GameState *game_state) {
    assert(game_state->initialised);

    Texture2D pitch_texture =
        assets_texture_get_by_id(game_state->texture_cache, game_state->pitch_texture_id)->texture;

    BeginTextureMode(game_state->render_texture);
    ClearBackground((Color){.r = 42, .g = 99, .b = 1, .a = 255});

    BeginMode2D(game_state->camera);

    f32 stretched_width = pitch_texture.width * 2.25f;
    f32 stretched_height = pitch_texture.height * 2.25f;
    DrawTexturePro(
        pitch_texture,
        (Rectangle){.x = 0, .y = 0, .width = pitch_texture.width, .height = pitch_texture.height},
        (Rectangle){.x = 0, .y = 0, .width = stretched_width, .height = stretched_height},
        Vector2Zero(), 0, WHITE);
    DrawTexturePro(
        pitch_texture,
        (Rectangle){.x = 0, .y = 0, .width = pitch_texture.width, .height = -pitch_texture.height},
        (Rectangle){
            .x = 0, .y = stretched_height, .width = stretched_width, .height = stretched_height},
        Vector2Zero(), 0, WHITE);
    DrawTexturePro(
        pitch_texture,
        (Rectangle){.x = 0, .y = 0, .width = -pitch_texture.width, .height = pitch_texture.height},
        (Rectangle){
            .x = stretched_width, .y = 0, .width = stretched_width, .height = stretched_height},
        Vector2Zero(), 0, WHITE);
    DrawTexturePro(
        pitch_texture,
        (Rectangle){.x = 0, .y = 0, .width = -pitch_texture.width, .height = -pitch_texture.height},
        (Rectangle){.x = stretched_width,
                    .y = stretched_height,
                    .width = stretched_width,
                    .height = stretched_height},
        Vector2Zero(), 0, WHITE);

    systems_render(game_state);
    systems_render_debug(game_state);
    EndMode2D();

    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);

    Rectangle rect = (Rectangle){.x = 0,
                                 .y = 0,
                                 .width = game_state->render_texture.texture.width,
                                 .height = -game_state->render_texture.texture.height};
    DrawTextureRec(game_state->render_texture.texture, rect, Vector2Zero(), WHITE);

    EndDrawing();
}
