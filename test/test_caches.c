#include "memory.h"
#include "as_animation.h"
#include "as_shader.h"
#include "as_sprite_frame.h"
#include "as_texture.h"
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
		memory_arena_create_default("test_arena", buffer, Megabytes(1));

	asset_texture_cache_t *handle = asset_texture_cache_create(memory_arena_handle);
	TEST_ASSERT(handle);
	struct asset_texture *assets_texture = asset_texture_load(handle, "res/spritesheets/ball.png");
	TEST_ASSERT_EQUAL_INT(0, strcmp(assets_texture->name, "res/spritesheets/ball.png"));
	TEST_ASSERT_GREATER_THAN(0, assets_texture->texture.id);

	struct asset_texture *assets_texture_ptr = asset_texture_get_by_id(handle, assets_texture->id);
	TEST_ASSERT_NOT_NULL(assets_texture_ptr);
	TEST_ASSERT_EQUAL_UINT(assets_texture->id, assets_texture_ptr->id);

	assets_texture_ptr = asset_texture_get_by_name(handle, "res/spritesheets/ball.png");
	assert(assets_texture_ptr != NULL);
	assert(assets_texture_ptr->id == assets_texture->id);

	asset_texture_cache_destroy(handle);

	GLOG_INFO(LS_TESTS, "texture cache test - end");
}

void test_sprite_cache()
{
	GLOG_INFO(LS_TESTS, "sprite frame cache test - start");

	void *buffer = malloc(Megabytes(1));
	memory_arena_handle_t *memory_arena_handle =
		memory_arena_create_default("test_arena", buffer, Megabytes(1));

	asset_sprite_frame_cache_t *handle = asset_sprite_frame_cache_create(memory_arena_handle);
	TEST_ASSERT(handle);
	i32 loaded = asset_sprite_frames_load_from_json(handle, 0, "res/spritesheets/ball.json");
	TEST_ASSERT_EQUAL_INT32(24, loaded);

	struct asset_sprite_frame *sprite_frame = asset_sprite_frame_get_by_name(handle, "ball_left_0");
	TEST_ASSERT_NOT_NULL(sprite_frame);
	TEST_ASSERT_EQUAL_INT(0, strcmp(sprite_frame->name, "ball_left_0"));
	TEST_ASSERT_EQUAL_UINT(0, sprite_frame->texture_id);

	sprite_frame = asset_sprite_frame_get_by_id(handle, sprite_frame->id);
	TEST_ASSERT_NOT_NULL(sprite_frame);
	TEST_ASSERT_EQUAL_INT(0, strcmp(sprite_frame->name, "ball_left_0"));
	TEST_ASSERT_EQUAL_UINT(0, sprite_frame->texture_id);

	asset_sprite_frame_cache_destroy(handle);
	GLOG_INFO(LS_TESTS, "sprite frame cache test - end");
}

void test_animation_cache()
{
	GLOG_INFO(LS_TESTS, "animation cache test - start");

	void *buffer = malloc(Megabytes(1));
	memory_arena_handle_t *memory_arena_handle =
		memory_arena_create_default("test_arena", buffer, Megabytes(1));

	asset_sprite_frame_cache_t *handle_sprite_cache =
		asset_sprite_frame_cache_create(memory_arena_handle);
	TEST_ASSERT(handle_sprite_cache);
	i32 loaded =
		asset_sprite_frames_load_from_json(handle_sprite_cache, 0, "res/spritesheets/ball.json");
	TEST_ASSERT_EQUAL_INT32(24, loaded);

	asset_animation_cache_t *handle = asset_animation_cache_create(memory_arena_handle);
	TEST_ASSERT(handle);
	loaded = asset_animations_load_from_json(
		handle, handle_sprite_cache, "res/animations/animations.json");
	TEST_ASSERT_EQUAL_INT32(64, loaded);

	struct asset_animation *animation = asset_animation_get_by_name(handle, "ball_left");
	TEST_ASSERT_NOT_NULL(animation);
	TEST_ASSERT_EQUAL_INT(0, strcmp(animation->name, "ball_left"));
	TEST_ASSERT_EQUAL_UINT(4, animation->frames_count);
	TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.05f, animation->delay);

	animation = asset_animation_get_by_id(handle, animation->id);
	TEST_ASSERT_NOT_NULL(animation);
	TEST_ASSERT_EQUAL_INT(0, strcmp(animation->name, "ball_left"));
	TEST_ASSERT_EQUAL_UINT(4, animation->frames_count);
	TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.05f, animation->delay);

	asset_animation_cache_destroy(handle);
	asset_sprite_frame_cache_destroy(handle_sprite_cache);

	GLOG_INFO(LS_TESTS, "animation cache test - end");
}

void test_shader_cache()
{
	GLOG_INFO(LS_TESTS, "shader cache test - start");

	InitWindow(800, 400, "test shader cache");

	void *buffer = malloc(Megabytes(1));
	memory_arena_handle_t *memory_arena_handle =
		memory_arena_create_default("test_arena", buffer, Megabytes(1));

	asset_shader_cache_t *handle = asset_shader_cache_create(memory_arena_handle);
	TEST_ASSERT(handle);
	struct asset_shader *assets_shader =
		asset_shader_load(handle, "player", "res/shaders/base.vert", "res/shaders/base.frag");
	TEST_ASSERT_EQUAL_INT(0, strcmp(assets_shader->name, "player"));
	TEST_ASSERT_GREATER_THAN(0, assets_shader->shader.id);

	struct asset_shader *assets_shader_ptr = asset_shader_get_by_id(handle, assets_shader->id);
	TEST_ASSERT_NOT_NULL(assets_shader_ptr);
	TEST_ASSERT_EQUAL_UINT(assets_shader->id, assets_shader_ptr->id);

	assets_shader_ptr = asset_shader_get_by_name(handle, "player");
	assert(assets_shader_ptr != NULL);
	assert(assets_shader_ptr->id == assets_shader->id);

	asset_shader_cache_destroy(handle);

	GLOG_INFO(LS_TESTS, "shader cache test - end");
}

int main()
{
	UNITY_BEGIN();
	RUN_TEST(test_texture_cache);
	RUN_TEST(test_sprite_cache);
	RUN_TEST(test_animation_cache);
	RUN_TEST(test_shader_cache);

	return UNITY_END();
}
