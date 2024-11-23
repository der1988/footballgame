#include "log.h"
#include "memory.h"
#include "scenes.h"
#include "types.h"
#include <assert.h>

struct scene_manager *scenes_manager_init(struct game_config config,
					  struct assets *assets,
					  memory_arena_handle_t *main_memory_arena_handle)
{
    // Scene manager will live as long as the game so we push it onto main memory arena
    struct scene_manager *handle =
	memory_arena_push(main_memory_arena_handle, sizeof(struct scene_manager));
    assert(handle);

    void *memory_for_current_scene = memory_arena_push(main_memory_arena_handle, Megabytes(4));

    handle->config = config;
    handle->current_scene_memory_arena =
	memory_arena_create("scene", memory_for_current_scene, Megabytes(4));
    handle->current_scene = NULL;
    handle->next_scene = SCT_GAME;
    handle->assets = assets;
    handle->initialised = true;

    GLOG_INFO(LS_SCENES, "scene manager initialised");

    return handle;
}

void scenes_manager_destroy(struct scene_manager *scene_manager)
{
    assert(scene_manager);
    assert(scene_manager->initialised);

    if (scene_manager->current_scene && scene_manager->current_scene->destroy_handler) {
	scene_manager->current_scene->destroy_handler(scene_manager->current_scene);
    }

    memory_arena_destroy(scene_manager->current_scene_memory_arena);

    GLOG_INFO(LS_SCENES, "scene manager destroyed");
}

void scenes_manager_load_scene(enum scene_type scene, struct scene_manager *scene_manager)
{
    scene_manager->next_scene = scene;
}

b32 scenes_manager_update(struct scene_manager *scene_manager)
{
    b32 quit = false;

    // Manage scene transitions: should this be front or back?
    if (scene_manager->current_scene == NULL ||
	scene_manager->next_scene != scene_manager->current_scene->type) {
	struct scene_header *scene = scene_manager->current_scene;
	if (scene && scene->destroy_handler) {
	    scene->destroy_handler(scene);
	    memory_arena_reset(scene_manager->current_scene_memory_arena);
	}

	switch (scene_manager->next_scene) {
	case SCT_TITLE:
	case SCT_MAIN:
	case SCT_OPTIONS:
	case SCT_GAME:
	    scene_manager->current_scene = (struct scene_header *)scenes_game_init(
		scene_manager, scene_manager->current_scene_memory_arena);
	    break;
	case SCT_COUNT:
	    break;
	}
    }

    switch (scene_manager->current_scene->type) {
    case SCT_TITLE:
    case SCT_MAIN:
    case SCT_OPTIONS:
    case SCT_GAME:
	quit = scenes_game_update((struct game_scene *)scene_manager->current_scene);
	break;
    case SCT_COUNT:
	break;
    }

    return quit;
}
