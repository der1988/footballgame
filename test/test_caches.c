#include "memory.h"
#include "assets.h"
#include "types.h"
#include "unity/unity.h"
#include "unity/unity_internals.h"
#include "log.h"
#include <assert.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setUp(void) {}

void tearDown(void) {}

void test_texture_cache()
{
    GLOG_INFO(LS_TESTS, "texture cache test - start");

    InitWindow(800, 400, "test texture cache");

    void *buffer = malloc(Megabytes(1));
    memory_arena_handle_t *memory_arena_handle =
	memory_arena_create("test_arena", buffer, Megabytes(1));

    assets_texture_cache_t *handle = assets_texture_cache_create(memory_arena_handle);
    TEST_ASSERT(handle);
    struct assets_texture *assets_texture =
	assets_texture_load(handle, "res/spritesheets/footballgame.png");
    TEST_ASSERT_EQUAL_INT(0, strcmp(assets_texture->name, "res/spritesheets/footballgame.png"));
    TEST_ASSERT_GREATER_THAN(0, assets_texture->texture.id);

    struct assets_texture *assets_texture_ptr =
	assets_texture_get_by_id(handle, assets_texture->id);
    TEST_ASSERT_NOT_NULL(assets_texture_ptr);
    TEST_ASSERT_EQUAL_UINT(assets_texture->id, assets_texture_ptr->id);

    assets_texture_ptr = assets_texture_get_by_name(handle, "res/spritesheets/footballgame.png");
    assert(assets_texture_ptr != NULL);
    assert(assets_texture_ptr->id == assets_texture->id);

    assets_texture_cache_destroy(handle);

    GLOG_INFO(LS_TESTS, "texture cache test - end");
}

void test_sprite_cache()
{
    GLOG_INFO(LS_TESTS, "sprite frame cache test - start");

    void *buffer = malloc(Megabytes(1));
    memory_arena_handle_t *memory_arena_handle =
	memory_arena_create("test_arena", buffer, Megabytes(1));

    assets_sprite_frame_cache_t *handle = assets_sprite_frame_cache_create(memory_arena_handle);
    TEST_ASSERT(handle);
    i32 loaded =
	assets_sprite_frames_load_from_json(handle, 0, "res/spritesheets/footballgame.json");
    TEST_ASSERT_EQUAL_INT32(161, loaded);

    struct assets_sprite_frame *sprite_frame =
	assets_sprite_frame_get_by_name(handle, "ball_left_0");
    TEST_ASSERT_NOT_NULL(sprite_frame);
    TEST_ASSERT_EQUAL_INT(0, strcmp(sprite_frame->name, "ball_left_0"));
    TEST_ASSERT_EQUAL_UINT(0, sprite_frame->texture_id);

    sprite_frame = assets_sprite_frame_get_by_id(handle, sprite_frame->id);
    TEST_ASSERT_NOT_NULL(sprite_frame);
    TEST_ASSERT_EQUAL_INT(0, strcmp(sprite_frame->name, "ball_left_0"));
    TEST_ASSERT_EQUAL_UINT(0, sprite_frame->texture_id);

    assets_sprite_frame_cache_destroy(handle);
    GLOG_INFO(LS_TESTS, "sprite frame cache test - end");
}

void test_animation_cache()
{
    GLOG_INFO(LS_TESTS, "animation cache test - start");

    void *buffer = malloc(Megabytes(1));
    memory_arena_handle_t *memory_arena_handle =
	memory_arena_create("test_arena", buffer, Megabytes(1));

    assets_sprite_frame_cache_t *handle_sprite_cache =
	assets_sprite_frame_cache_create(memory_arena_handle);
    TEST_ASSERT(handle_sprite_cache);
    i32 loaded = assets_sprite_frames_load_from_json(
	handle_sprite_cache, 0, "res/spritesheets/footballgame.json");
    TEST_ASSERT_EQUAL_INT32(161, loaded);

    assets_animation_cache_t *handle = assets_animation_cache_create(memory_arena_handle);
    TEST_ASSERT(handle);
    loaded = assets_animations_load_from_json(
	handle, handle_sprite_cache, "res/animations/animations.json");
    TEST_ASSERT_EQUAL_INT32(36, loaded);

    struct assets_animation *animation = assets_animation_get_by_name(handle, "ball_left");
    TEST_ASSERT_NOT_NULL(animation);
    TEST_ASSERT_EQUAL_INT(0, strcmp(animation->name, "ball_left"));
    TEST_ASSERT_EQUAL_UINT(4, animation->frames_count);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.05f, animation->delay);

    animation = assets_animation_get_by_id(handle, animation->id);
    TEST_ASSERT_NOT_NULL(animation);
    TEST_ASSERT_EQUAL_INT(0, strcmp(animation->name, "ball_left"));
    TEST_ASSERT_EQUAL_UINT(4, animation->frames_count);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.05f, animation->delay);

    assets_animation_cache_destroy(handle);
    assets_sprite_frame_cache_destroy(handle_sprite_cache);

    GLOG_INFO(LS_TESTS, "animation cache test - end");
}

int main()
{
    UNITY_BEGIN();
    RUN_TEST(test_texture_cache);
    RUN_TEST(test_sprite_cache);
    RUN_TEST(test_animation_cache);

    return UNITY_END();
}
