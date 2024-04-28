// clang-format off
#include "memory/memory.h"
#include "assets/assets.h"
// clang-format on
#include "actions/actions.h"
#include "components/components.h"
#include "entities/entities.h"
#include "game.h"
#include "types.h"
#include "unity/unity.h"
#include "unity/unity_internals.h"
#include "utils/utils.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GameState *g_game_state = NULL;

void setUp(void) {
    int width = 800;
    int height = 600;
    InitWindow(width, height, "Test Entity Repos");

    SetTargetFPS(60);

    void *memory = malloc(Megabytes(10));

    GameConfig game_config =
        (GameConfig){.screen_width = width, .screen_height = height, .memory = memory};

    g_game_state = game_init(game_config);
}

void tearDown(void) {
    void *memory = g_game_state->config.memory;

    game_destroy(g_game_state);

    free(memory);

    CloseWindow();
}

void test_entity() {
    struct EntityRepository *entity_repo = g_game_state->entity_repository;
    struct Entity *entity =
        entities_player_create(g_game_state, (PlayerConfig){.position = Vector2Zero()});
    TEST_ASSERT_NOT_NULL(entity);
    EntityId entity_id = entity->id;

    entity = entities_entity_get(entity_repo, entity_id);
    TEST_ASSERT_NOT_NULL(entity);

    Component *transform = entities_component_get(entity_repo, CT_TRANSFORM, entity_id);
    TEST_ASSERT_NOT_NULL(transform);
    Component *physics = entities_component_get(entity_repo, CT_PHYSICS, entity_id);
    TEST_ASSERT_NOT_NULL(physics);
    Component *sprite = entities_component_get(entity_repo, CT_SPRITE, entity_id);
    TEST_ASSERT_NOT_NULL(sprite);

    entities_entity_destroy(entity_repo, entity_id);

    entity = entities_entity_get(entity_repo, entity_id);
    TEST_ASSERT_NULL(entity);
    transform = entities_component_get(entity_repo, CT_TRANSFORM, entity_id);
    TEST_ASSERT_NULL(transform);
    physics = entities_component_get(entity_repo, CT_PHYSICS, entity_id);
    TEST_ASSERT_NULL(physics);
    sprite = entities_component_get(entity_repo, CT_SPRITE, entity_id);
    TEST_ASSERT_NULL(sprite);
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_entity);

    return UNITY_END();
}
