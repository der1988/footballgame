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
#include <stdlib.h>

GameState *game_init(GameConfig game_config) {
    MemoryArenaHandle *memory_arena_lifetime_game =
        memory_arena_create("game", game_config.memory, Megabytes(1));

    GameState *game_state = memory_arena_push(memory_arena_lifetime_game, sizeof(GameState));
    game_state->config = game_config;
    game_state->config.players_count = 1;

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
    AssetsTexture *texture =
        assets_texture_load(game_state->texture_cache, "res/spritesheets/footballgame.png");
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
    //
    // Entity *soldier = entities_soldier_create(game_state);
    // Component *soldier_transform =
    //     entities_component_get(game_state->entity_repository, CT_TRANSFORM, soldier->id);
    // soldier_transform->transform.position = (Vector2){game_state->config.screen_width / 2.0f, 0};
    //
    // Entity *barracks = entities_barracks_create(game_state);
    // Component *barracks_transform =
    //     entities_component_get(game_state->entity_repository, CT_TRANSFORM, barracks->id);
    //
    // barracks_transform->transform.position = (Vector2){game_state->config.screen_width / 2.0f,
    // 0}; aircraft_transform_1->transform.position =
    //     (Vector2){game_state->config.screen_width / 2.0f - 100.0f, 0};
    // aircraft_transform_2->transform.position =
    //     (Vector2){game_state->config.screen_width / 2.0f + 100.0f, 0};
    //
    // Entity *terrain = entities_terrain_create(game_state);

    Entity *camera = entities_camera_create(game_state);
    ComponentCameraProps *camera_props =
        &entities_component_get(game_state->entity_repository, CT_CAMERA_PROPS, camera->id)
             ->camera_props;
    ComponentTarget *camera_target =
        &entities_component_get(game_state->entity_repository, CT_TARGET, camera->id)->target;
    camera_props->camera = CAM_MAIN;

    game_state->render_texture = LoadRenderTexture((f32)game_state->config.screen_width,
                                                   (f32)game_state->config.screen_height);
    game_state->camera.zoom = 1;
    game_state->camera.rotation = 0;
    game_state->camera.offset = (Vector2){(f32)game_state->config.screen_width / 2,
                                          (f32)game_state->config.screen_height / 2};
    camera_target->target_transform = player_transform;
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

    EntityForEach(game_state->entity_repository, entity, idx) {
        if (entity->alive && EntityTypeCheckMask(entity->type, EntityTypeAllBitMask)) {
            entities_entity_update(entity, game_state, input);
        }
    }

#ifndef RUNNING_TEST
#endif

    systems_action(game_state);
    systems_camera(game_state);
}

void game_draw(GameState *game_state) {
    assert(game_state->initialised);

    BeginTextureMode(game_state->render_texture);
    ClearBackground((Color){.r = 131, .g = 147, .b = 178, .a = 255});
    // ClearBackground(BLACK);

    BeginMode2D(game_state->camera);
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
