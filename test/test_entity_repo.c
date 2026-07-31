#include "asset_manager.h"
#include "entity_components.h"
#include "entity.h"
#include "entity_player.h"
#include "game.h"
#include "types.h"
#include "unity/unity.h"
#include "unity/unity_internals.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct game_state *g_game_state = NULL;
void *g_memory = NULL;
memory_arena_handle_t *g_main_memory_arena = NULL;

void setUp(void)
{
	int width = 800;
	int height = 600;
	InitWindow(width, height, "Test Entity Repos");

	SetTargetFPS(60);

	g_memory = malloc(Megabytes(128));

	g_main_memory_arena = memory_arena_create_default("main", g_memory, Megabytes(128));

	struct assets assets = asset_manager_load_everything(g_main_memory_arena);

	struct game_config game_config =
		(struct game_config){.screen_width = width, .screen_height = height};

	g_game_state = game_play_init(game_config, assets, g_main_memory_arena);
}

void tearDown(void)
{
	game_play_destroy(g_game_state);

	memory_arena_destroy(g_main_memory_arena);

	free(g_memory);

	CloseWindow();
}

void test_entity()
{
	struct entity_repository *entity_repo = g_game_state->entity_repository;
	struct entity *entity =
		entity_player_create((struct player_config){.position = Vector2Zero()}, g_game_state);
	TEST_ASSERT_NOT_NULL(entity);
	entity_id_t entity_id = entity->header.id;

	entity = entity_get(entity_repo, entity_id);
	TEST_ASSERT_NOT_NULL(entity);

	TEST_ASSERT(entity->transform.header.enabled);
	TEST_ASSERT(entity->physics.header.enabled);
	TEST_ASSERT(entity->sprite.header.enabled);

	entity_destroy(entity_repo, entity_id);

	entity = entity_get(entity_repo, entity_id);
	TEST_ASSERT(!entity->header.alive);
}

int main()
{
	UNITY_BEGIN();

	RUN_TEST(test_entity);

	return UNITY_END();
}
