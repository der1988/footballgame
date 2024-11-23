#include "memory.h"
#include "types.h"
#include "game.h"
#include "assets.h"

#define MAX_SCENES 5

struct game_scene;

enum scene_state {
    SS_INITIALIZED,
    SS_SHOWN,
    SS_HIDDEN,
};

enum scene_type {
    SCT_TITLE,
    SCT_MAIN,
    SCT_OPTIONS,
    SCT_GAME,
    SCT_COUNT
};

struct scene_header {

    enum scene_type type;
    enum scene_state state;
    void (*destroy_handler)(struct scene_header *);
    void (*show_handler)();
    void (*hide_handler)();
};

struct scene_manager {
    b32 initialised;
    struct game_config config;
    memory_arena_handle_t *current_scene_memory_arena;
    struct scene_header *current_scene;
    enum scene_type next_scene;
    struct assets *assets;
};

struct scene_manager *scenes_manager_init(struct game_config config,
					  struct assets *assets,
					  memory_arena_handle_t *main_memory_arena_handle);
void scenes_manager_destroy(struct scene_manager *scene_manager);
void scenes_manager_load_scene(enum scene_type scene, struct scene_manager *scene_manager);
b32 scenes_manager_update(struct scene_manager *scene_manager);

struct game_scene {
    struct scene_header header;
    f64 current_time;
    f64 accumulator;
    f64 dt;
    struct game_state *state;
};

struct game_scene *scenes_game_init(struct scene_manager *scene_manager,
				    memory_arena_handle_t *scene_memory_arena);
void scenes_game_destroy(struct scene_header *scene_header);
b32 scenes_game_update(struct game_scene *game_scene);
