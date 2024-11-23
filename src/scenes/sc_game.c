#include "game.h"
#include "input.h"
#include "log.h"
#include "memory.h"
#include "scenes.h"
#include <assert.h>

struct game_scene *scenes_game_init(struct scene_manager *scene_manager,
				    memory_arena_handle_t *scene_memory_arena)
{
    struct game_scene *handle = memory_arena_push(scene_memory_arena, sizeof(struct game_scene));
    assert(handle);

    handle->header.type = SCT_GAME;
    handle->header.state = SS_INITIALIZED;
    handle->header.destroy_handler = scenes_game_destroy;

    handle->state = game_init(scene_manager->config, scene_manager->assets, scene_memory_arena);

    handle->current_time = GetTime();
    handle->accumulator = 0.0;
    handle->dt = handle->state->constants.fixed_timestep;

    GLOG_INFO(LS_SCENES, "game scene initialised");

    return handle;
}

void scenes_game_destroy(struct scene_header *scene_header)
{
    assert(scene_header);

    game_destroy(((struct game_scene *)scene_header)->state);

    GLOG_INFO(LS_SCENES, "game scene destroyed");
}

b32 scenes_game_update(struct game_scene *game_scene)
{
    struct game_state *game_state = game_scene->state;

    b32 quit = false;

    if (game_state->vars.is_tweakui_shown) {
	PollInputEvents();
	quit = WindowShouldClose();
	if (IsKeyReleased(KEY_P)) {
	    game_state->vars.is_tweakui_shown = false;
	}
    } else {
	// Update game
	f64 new_time = GetTime();
	f64 frame_time = new_time - game_scene->current_time;
	f64 total_update_time = 0;
	game_scene->current_time = new_time;

	if (frame_time > 1) {
	    frame_time = game_state->dt;
	}

	u32 physics_iterations = 0;
	game_scene->accumulator += frame_time;
	while (game_scene->accumulator > 0) {
	    PollInputEvents();
	    quit |= WindowShouldClose();

	    // TODO: (Marco) Move this stuff away for god sake
	    if (IsKeyReleased(KEY_P)) {
		game_state->vars.is_tweakui_shown = true;
	    }
	    if (IsKeyReleased(KEY_C)) {
		game_state->vars.is_debug_log_shown = !game_state->vars.is_debug_log_shown;
	    }

	    struct input input = input_handle(game_state);

	    game_state->dt = game_scene->dt;

	    f64 update_time = GetTime();
	    game_update(game_state, &input);
	    total_update_time += GetTime() - update_time;

	    game_scene->accumulator -= game_scene->dt;
	    game_state->time += game_scene->dt;

	    physics_iterations++;
	    game_state->physics_updates++;
	}

	game_state->frame_updates++;

	if (physics_iterations > 0) {
	    layers_debug_queue_draw_text(game_state->layer_debug,
					 TextFormat("Update time: %0.6f",
						    total_update_time / physics_iterations));
	    layers_debug_queue_draw_text(game_state->layer_debug,
					 TextFormat("Physics/frame: %2d", physics_iterations));
	    layers_debug_queue_draw_text(game_state->layer_debug,
					 TextFormat("Physics: %12d", game_state->physics_updates));
	    layers_debug_queue_draw_text(game_state->layer_debug,
					 TextFormat("Frame: %14d", game_state->frame_updates));
	    layers_debug_queue_draw_text(game_state->layer_debug,
					 TextFormat("FPS: %3.0f", 1 / frame_time));
	}
    }

    game_draw(game_state);

    return quit;
}
