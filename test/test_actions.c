// clang-format off
#include "../src/memory/memory.h"
#include "../src/assets/assets.h"
// clang-format on
#include "../src/types.h"
#include "actions/actions.h"
#include "components/components.h"
#include "config.h"
#include "entities/entities.h"
#include "game.h"
#include "systems/systems.h"
#include "unity/unity.h"
#include "unity/unity_internals.h"
#include "utils/utils.h"
#include <assert.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DT (0.1f)

GameState *g_game_state = NULL;

void setUp(void) {
    int width = 800;
    int height = 600;
    InitWindow(width, height, "Test Actions");

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

void test_action_change_color_from_to() {
    struct Entity *entity =
        entities_player_create(g_game_state, (PlayerConfig){.position = Vector2Zero()});
    TEST_ASSERT_NOT_NULL(entity);
    EntityId entity_id = entity->id;

    ComponentAction *component_action =
        entities_component_create(g_game_state->entity_repository, CT_ACTION, entity_id);
    ComponentSprite *component_sprite =
        entities_component_create(g_game_state->entity_repository, CT_SPRITE, entity_id);

    ActionChangeColorFromTo *action = actions_change_color_from_to_create(
        1, false, BLACK, WHITE, g_game_state->memory_by_lifetime[MAL_LEVEL]);

    RunningActionIdx running_idx =
        components_action_run(entity_id, (struct Action *)action, g_game_state);

    g_game_state->dt = DT;
    usize iterations = ceilf(action->base.duration / g_game_state->dt) + 1;
    g_game_state->time = g_game_state->dt;
    for (usize i = 0; i < iterations; ++i) {
        systems_action(g_game_state);
        g_game_state->time += g_game_state->dt;
    }

    TEST_ASSERT(component_action->running_actions[running_idx].completed);
    Color expected_color = WHITE;
    TEST_ASSERT_EQUAL_UINT8(expected_color.r, component_sprite->color.r);
    TEST_ASSERT_EQUAL_UINT8(expected_color.g, component_sprite->color.g);
    TEST_ASSERT_EQUAL_UINT8(expected_color.b, component_sprite->color.b);
    TEST_ASSERT_EQUAL_UINT8(expected_color.a, component_sprite->color.a);
}

void test_action_animation() {
    struct Entity *entity =
        entities_player_create(g_game_state, (PlayerConfig){.position = Vector2Zero()});
    TEST_ASSERT_NOT_NULL(entity);
    EntityId entity_id = entity->id;

    ComponentAction *component_action =
        entities_component_create(g_game_state->entity_repository, CT_ACTION, entity_id);
    ComponentSprite *component_sprite =
        entities_component_create(g_game_state->entity_repository, CT_SPRITE, entity_id);

    AssetsAnimation *animation =
        assets_animation_get_by_name(g_game_state->animation_cache, "ball_left");

    ActionPlayAnimation *action = actions_play_animation_create(
        animation, false, 1.0f, g_game_state->memory_by_lifetime[MAL_LEVEL]);

    RunningActionIdx running_idx =
        components_action_run(entity_id, (struct Action *)action, g_game_state);

    g_game_state->dt = DT;
    usize iterations = ceilf(action->base.duration / g_game_state->dt) + 1;
    g_game_state->time = 0;
    for (usize i = 0; i < iterations; ++i) {
        systems_action(g_game_state);
        g_game_state->time += g_game_state->dt;
        if (component_action->running_actions[running_idx].completed && i != (iterations - 1)) {
            TEST_FAIL();
        }
    }

    TEST_ASSERT(component_action->running_actions[running_idx].completed);
    AssetsSpriteFrame *expected_sprite_frame =
        assets_sprite_frame_get_by_name(g_game_state->sprite_frame_cache, "ball_left_3");
    TEST_ASSERT_EQUAL_UINT(expected_sprite_frame->id, component_sprite->frame_id);

    // Test half play speed
    action = actions_play_animation_create(animation, false, 0.5f,
                                           g_game_state->memory_by_lifetime[MAL_LEVEL]);

    running_idx = components_action_run(entity_id, (struct Action *)action, g_game_state);

    g_game_state->dt = DT;
    iterations = ceilf(action->base.duration / g_game_state->dt) + 1;
    g_game_state->time = 0;
    for (usize i = 0; i < iterations; ++i) {
        systems_action(g_game_state);
        g_game_state->time += g_game_state->dt;
        if (component_action->running_actions[running_idx].completed && i != (iterations - 1)) {
            TEST_FAIL();
        }
    }

    TEST_ASSERT(component_action->running_actions[running_idx].completed);
    expected_sprite_frame =
        assets_sprite_frame_get_by_name(g_game_state->sprite_frame_cache, "ball_left_3");
    TEST_ASSERT_EQUAL_UINT(expected_sprite_frame->id, component_sprite->frame_id);

    // Test double play speed
    action = actions_play_animation_create(animation, false, 2.0f,
                                           g_game_state->memory_by_lifetime[MAL_LEVEL]);

    running_idx = components_action_run(entity_id, (struct Action *)action, g_game_state);

    g_game_state->dt = DT;
    iterations = ceilf(action->base.duration / g_game_state->dt) + 1;
    g_game_state->time = 0;
    for (usize i = 0; i < iterations; ++i) {
        systems_action(g_game_state);
        g_game_state->time += g_game_state->dt;
        if (component_action->running_actions[running_idx].completed && i != (iterations - 1)) {
            TEST_FAIL();
        }
    }

    TEST_ASSERT(component_action->running_actions[running_idx].completed);
    expected_sprite_frame =
        assets_sprite_frame_get_by_name(g_game_state->sprite_frame_cache, "ball_left_3");
    TEST_ASSERT_EQUAL_UINT(expected_sprite_frame->id, component_sprite->frame_id);
}

void test_action_sequence() {
    struct Entity *entity =
        entities_player_create(g_game_state, (PlayerConfig){.position = Vector2Zero()});
    TEST_ASSERT_NOT_NULL(entity);
    EntityId entity_id = entity->id;

    ComponentAction *component_action =
        entities_component_create(g_game_state->entity_repository, CT_ACTION, entity_id);
    ComponentSprite *component_sprite =
        entities_component_create(g_game_state->entity_repository, CT_SPRITE, entity_id);

    MemoryArenaHandle *memory_arena_handle = g_game_state->memory_by_lifetime[MAL_LEVEL];
    ActionChangeColorFromTo *action_1 =
        actions_change_color_from_to_create(1, false, WHITE, BLACK, memory_arena_handle);
    ActionChangeColorFromTo *action_2 =
        actions_change_color_from_to_create(1, false, BLACK, RED, memory_arena_handle);
    ActionSequence *action_sequence =
        actions_sequence_create(memory_arena_handle, 2, action_1, action_2);

    RunningActionIdx running_idx =
        components_action_run(entity_id, (struct Action *)action_sequence, g_game_state);

    g_game_state->dt = DT;
    usize iterations = ceilf(action_sequence->base.duration / g_game_state->dt) + 1;
    g_game_state->time = g_game_state->dt;
    for (usize i = 0; i < iterations; ++i) {
        systems_action(g_game_state);
        g_game_state->time += g_game_state->dt;
        if (component_action->running_actions[running_idx].completed && i != (iterations - 1)) {
            TEST_FAIL();
        }
    }

    TEST_ASSERT(component_action->running_actions[running_idx].completed);
    Color expected_color = RED;
    TEST_ASSERT_EQUAL_UINT8(expected_color.r, component_sprite->color.r);
    TEST_ASSERT_EQUAL_UINT8(expected_color.g, component_sprite->color.g);
    TEST_ASSERT_EQUAL_UINT8(expected_color.b, component_sprite->color.b);
    TEST_ASSERT_EQUAL_UINT8(expected_color.a, component_sprite->color.a);
}

void test_action_spawn() {
    struct Entity *entity =
        entities_player_create(g_game_state, (PlayerConfig){.position = Vector2Zero()});
    TEST_ASSERT_NOT_NULL(entity);
    EntityId entity_id = entity->id;
    ComponentAction *component_action =
        entities_component_create(g_game_state->entity_repository, CT_ACTION, entity_id);
    ComponentSprite *component_sprite =
        entities_component_create(g_game_state->entity_repository, CT_SPRITE, entity_id);

    AssetsAnimation *animation =
        assets_animation_get_by_name(g_game_state->animation_cache, "ball_left");

    ActionPlayAnimation *action_1 = actions_play_animation_create(
        animation, false, 1.0f, g_game_state->memory_by_lifetime[MAL_LEVEL]);

    ActionChangeColorFromTo *action_2 = actions_change_color_from_to_create(
        action_1->base.duration, false, BLACK, WHITE, g_game_state->memory_by_lifetime[MAL_LEVEL]);

    ActionSpawn *action_spawn =
        actions_spawn_create(g_game_state->memory_by_lifetime[MAL_LEVEL], 2, action_1, action_2);

    RunningActionIdx action_spawn_idx =
        components_action_run(entity_id, (struct Action *)action_spawn, g_game_state);

    g_game_state->dt = DT;
    usize iterations = ceilf(action_spawn->base.duration / g_game_state->dt) + 1;
    g_game_state->time = g_game_state->dt;
    for (usize i = 0; i < iterations; ++i) {
        systems_action(g_game_state);
        g_game_state->time += g_game_state->dt;
        if (component_action->running_actions[action_spawn_idx].completed &&
            i != (iterations - 1)) {
            TEST_FAIL();
        }
    }

    TEST_ASSERT(component_action->running_actions[action_spawn_idx].completed);

    AssetsSpriteFrame *expected_sprite_frame =
        assets_sprite_frame_get_by_name(g_game_state->sprite_frame_cache, "ball_left_3");
    TEST_ASSERT_EQUAL_UINT(expected_sprite_frame->id, component_sprite->frame_id);

    Color expected_color = WHITE;
    TEST_ASSERT_EQUAL_UINT8(expected_color.r, component_sprite->color.r);
    TEST_ASSERT_EQUAL_UINT8(expected_color.g, component_sprite->color.g);
    TEST_ASSERT_EQUAL_UINT8(expected_color.b, component_sprite->color.b);
    TEST_ASSERT_EQUAL_UINT8(expected_color.a, component_sprite->color.a);
}

void test_multiple_parallel_actions() {
    struct Entity *entity =
        entities_player_create(g_game_state, (PlayerConfig){.position = Vector2Zero()});
    TEST_ASSERT_NOT_NULL(entity);
    EntityId entity_id = entity->id;
    ComponentAction *component_action =
        entities_component_create(g_game_state->entity_repository, CT_ACTION, entity_id);
    ComponentSprite *component_sprite =
        entities_component_create(g_game_state->entity_repository, CT_SPRITE, entity_id);

    AssetsAnimation *animation =
        assets_animation_get_by_name(g_game_state->animation_cache, "ball_left");

    ActionPlayAnimation *action_1 = actions_play_animation_create(
        animation, false, 1.0f, g_game_state->memory_by_lifetime[MAL_LEVEL]);

    ActionChangeColorFromTo *action_2 = actions_change_color_from_to_create(
        action_1->base.duration, false, BLACK, WHITE, g_game_state->memory_by_lifetime[MAL_LEVEL]);

    RunningActionIdx animation_idx =
        components_action_run(entity_id, (struct Action *)action_1, g_game_state);
    RunningActionIdx change_color_idx =
        components_action_run(entity_id, (struct Action *)action_2, g_game_state);

    g_game_state->dt = DT;
    usize iterations = ceilf(action_1->base.duration / g_game_state->dt) + 1;
    g_game_state->time = g_game_state->dt;
    for (usize i = 0; i < iterations; ++i) {
        systems_action(g_game_state);
        g_game_state->time += g_game_state->dt;
        if (component_action->running_actions[animation_idx].completed && i != (iterations - 1)) {
            TEST_FAIL();
        }
        if (component_action->running_actions[change_color_idx].completed &&
            i != (iterations - 1)) {
            TEST_FAIL();
        }
    }

    TEST_ASSERT(component_action->running_actions[animation_idx].completed);
    AssetsSpriteFrame *expected_sprite_frame =
        assets_sprite_frame_get_by_name(g_game_state->sprite_frame_cache, "ball_left_3");
    TEST_ASSERT_EQUAL_UINT(expected_sprite_frame->id, component_sprite->frame_id);

    TEST_ASSERT(component_action->running_actions[change_color_idx].completed);
    Color expected_color = WHITE;
    TEST_ASSERT_EQUAL_UINT8(expected_color.r, component_sprite->color.r);
    TEST_ASSERT_EQUAL_UINT8(expected_color.g, component_sprite->color.g);
    TEST_ASSERT_EQUAL_UINT8(expected_color.b, component_sprite->color.b);
    TEST_ASSERT_EQUAL_UINT8(expected_color.a, component_sprite->color.a);
}

void test_action_pause() {
    struct Entity *entity =
        entities_player_create(g_game_state, (PlayerConfig){.position = Vector2Zero()});
    TEST_ASSERT_NOT_NULL(entity);
    EntityId entity_id = entity->id;

    ComponentAction *component_action =
        entities_component_create(g_game_state->entity_repository, CT_ACTION, entity_id);
    ComponentSprite *component_sprite =
        entities_component_create(g_game_state->entity_repository, CT_SPRITE, entity_id);

    ActionChangeColorFromTo *action = actions_change_color_from_to_create(
        1, false, BLACK, WHITE, g_game_state->memory_by_lifetime[MAL_LEVEL]);

    RunningActionIdx running_idx =
        components_action_run(entity_id, (struct Action *)action, g_game_state);

    g_game_state->dt = DT;
    usize iterations = ceilf(action->base.duration / g_game_state->dt) + 1;
    g_game_state->time = g_game_state->dt;
    usize first_batch = iterations / 2;
    for (usize i = 0; i < first_batch; ++i) {
        systems_action(g_game_state);
        g_game_state->time += g_game_state->dt;
    }
    component_action->running_actions[running_idx].paused = true;
    for (usize i = 0; i < 100; ++i) {
        systems_action(g_game_state);
        g_game_state->time += g_game_state->dt;
    }
    TEST_ASSERT(!component_action->running_actions[running_idx].completed);
    TEST_ASSERT_EQUAL_UINT8(102, component_sprite->color.r);
    TEST_ASSERT_EQUAL_UINT8(102, component_sprite->color.g);
    TEST_ASSERT_EQUAL_UINT8(102, component_sprite->color.b);
    TEST_ASSERT_EQUAL_UINT8(255, component_sprite->color.a);

    component_action->running_actions[running_idx].paused = false;
    usize second_batch = iterations - first_batch;
    for (usize i = 0; i < second_batch; ++i) {
        systems_action(g_game_state);
        g_game_state->time += g_game_state->dt;
    }

    TEST_ASSERT(component_action->running_actions[running_idx].completed);
    Color expected_color = WHITE;
    TEST_ASSERT_EQUAL_UINT8(expected_color.r, component_sprite->color.r);
    TEST_ASSERT_EQUAL_UINT8(expected_color.g, component_sprite->color.g);
    TEST_ASSERT_EQUAL_UINT8(expected_color.b, component_sprite->color.b);
    TEST_ASSERT_EQUAL_UINT8(expected_color.a, component_sprite->color.a);
}

void test_action_tween_float_from_to() {
    struct Entity *entity =
        entities_player_create(g_game_state, (PlayerConfig){.position = Vector2Zero()});
    TEST_ASSERT_NOT_NULL(entity);
    EntityId entity_id = entity->id;

    ComponentAction *component_action =
        entities_component_create(g_game_state->entity_repository, CT_ACTION, entity_id);

    f32 value = 0.0f;
    ActionTweenFloatFromTo *action = actions_tween_float_from_to_create(
        1, false, &value, 22.2f, 65.4f, g_game_state->memory_by_lifetime[MAL_LEVEL]);

    RunningActionIdx running_idx =
        components_action_run(entity_id, (struct Action *)action, g_game_state);

    g_game_state->dt = DT;
    usize iterations = ceilf(action->base.duration / g_game_state->dt) + 1;
    g_game_state->time = g_game_state->dt;
    for (usize i = 0; i < iterations; ++i) {
        systems_action(g_game_state);
        g_game_state->time += g_game_state->dt;
    }

    TEST_ASSERT(component_action->running_actions[running_idx].completed);
    TEST_ASSERT_EQUAL_FLOAT(65.4f, value);
}

void test_action_rotate_to() {
    struct Entity *entity =
        entities_player_create(g_game_state, (PlayerConfig){.position = Vector2Zero()});
    TEST_ASSERT_NOT_NULL(entity);
    EntityId entity_id = entity->id;

    ComponentTransform *transform =
        entities_component_create(g_game_state->entity_repository, CT_TRANSFORM, entity_id);
    ComponentAction *component_action =
        entities_component_create(g_game_state->entity_repository, CT_ACTION, entity_id);

    transform->rotation = 720.0f;
    ActionRotateTo *action =
        actions_rotate_to_create(1, false, 74.3f, g_game_state->memory_by_lifetime[MAL_LEVEL]);

    RunningActionIdx running_idx =
        components_action_run(entity_id, (struct Action *)action, g_game_state);

    g_game_state->dt = DT;
    usize iterations = ceilf(action->base.duration / g_game_state->dt) + 1;
    g_game_state->time = g_game_state->dt;
    for (usize i = 0; i < iterations; ++i) {
        systems_action(g_game_state);
        g_game_state->time += g_game_state->dt;
    }

    TEST_ASSERT(component_action->running_actions[running_idx].completed);
    TEST_ASSERT_EQUAL_FLOAT(74.3f, utils_math_constrain_angle_360(transform->rotation));
}

void test_action_toggle_boolean() {
    struct Entity *entity =
        entities_player_create(g_game_state, (PlayerConfig){.position = Vector2Zero()});
    TEST_ASSERT_NOT_NULL(entity);
    EntityId entity_id = entity->id;

    ComponentAction *component_action =
        entities_component_create(g_game_state->entity_repository, CT_ACTION, entity_id);

    b32 value = false;
    ActionToggleBoolean *action = actions_toggle_boolean_create(
        1, false, &value, g_game_state->memory_by_lifetime[MAL_LEVEL]);

    RunningActionIdx running_idx =
        components_action_run(entity_id, (struct Action *)action, g_game_state);

    g_game_state->dt = DT;
    g_game_state->time = g_game_state->dt;
    for (usize i = 0; i < 1; ++i) {
        systems_action(g_game_state);
        g_game_state->time += g_game_state->dt;
    }

    TEST_ASSERT(component_action->running_actions[running_idx].completed);
    TEST_ASSERT(value);
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_action_animation);
    RUN_TEST(test_action_spawn);
    RUN_TEST(test_multiple_parallel_actions);
    RUN_TEST(test_action_change_color_from_to);
    RUN_TEST(test_action_sequence);
    RUN_TEST(test_action_pause);
    RUN_TEST(test_action_tween_float_from_to);
    RUN_TEST(test_action_rotate_to);
    RUN_TEST(test_action_toggle_boolean);

    return UNITY_END();
}
