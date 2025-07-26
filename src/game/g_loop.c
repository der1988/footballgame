#include "memory.h"
#include "types.h"
#include "game.h"

enum game_scene g_next_scene = GSC_NONE;

static void *handle_scene_creation(struct game_config config,
								   struct assets assets,
								   memory_arena_handle_t *memory_arena);

static void handle_scene_destruction(enum game_scene current_scene, void *handle);

void game_loop(struct game_config config,
			   struct assets assets,
			   memory_arena_handle_t *main_memory_arena_handle)
{
	void *memory_for_current_scene = memory_arena_push(main_memory_arena_handle, Megabytes(100));

	memory_arena_handle_t *current_scene_memory_arena =
		memory_arena_create_default("scene", memory_for_current_scene, Megabytes(100));

	enum game_scene current_scene = GSC_NONE;
	void *current_scene_handle = NULL;
	g_next_scene = GSC_PLAY;

	b32 quit = false;

	while (!quit) {

		// Manage scene transitions
		if (g_next_scene != GSC_NONE && g_next_scene != current_scene) {
			// Destroy current scene
			handle_scene_destruction(current_scene, current_scene_handle);

			// Reset scene memory arena
			memory_arena_reset(current_scene_memory_arena);

			// Initialise new scene
			current_scene_handle =
				handle_scene_creation(config, assets, current_scene_memory_arena);

			current_scene = g_next_scene;
			g_next_scene = GSC_NONE;
		}

		switch (current_scene) {
		case GSC_TITLE:
		case GSC_MAIN:
		case GSC_PLAY: {
			quit = game_play_update(current_scene_handle);
		} break;
		case GSC_SETTINGS:
		case GSC_NONE:
			break;
		}

		SwapScreenBuffer();
	}

	handle_scene_destruction(current_scene, current_scene_handle);

	memory_arena_destroy(current_scene_memory_arena);
}

void game_set_next_scene(enum game_scene next_scene) { g_next_scene = next_scene; }

static void *handle_scene_creation(struct game_config config,
								   struct assets assets,
								   memory_arena_handle_t *memory_arena)
{
	void *current_scene_handle = NULL;

	switch (g_next_scene) {
	case GSC_TITLE:
	case GSC_MAIN:
	case GSC_PLAY: {
		current_scene_handle = game_play_init(config, assets, memory_arena);
	} break;
	case GSC_SETTINGS:
	case GSC_NONE:
		break;
	}

	return current_scene_handle;
}

static void handle_scene_destruction(enum game_scene current_scene, void *handle)
{
	// Destroy current scene
	switch (current_scene) {
	case GSC_TITLE:
	case GSC_MAIN:
	case GSC_PLAY: {
		game_play_destroy(handle);
	} break;
	case GSC_SETTINGS:
	case GSC_NONE:
		break;
	}
}
