#include "input.h"
#include "memory.h"
#include "types.h"
#include "game.h"

enum game_scene {
    GSC_NONE,
    GSC_TITLE,
    GSC_MAIN,
    GSC_SETTINGS,
    GSC_PLAY,
};

enum game_scene g_next_scene = GSC_NONE;

void *handle_scene_creation(struct game_config config,
			    struct assets assets,
			    memory_arena_handle_t *memory_arena);

void handle_scene_destruction(enum game_scene current_scene, void *handle);

void game_loop(struct game_config config,
	       struct assets assets,
	       memory_arena_handle_t *main_memory_arena_handle)
{
    void *memory_for_current_scene = memory_arena_push(main_memory_arena_handle, Megabytes(4));

    memory_arena_handle_t *current_scene_memory_arena =
	memory_arena_create("scene", memory_for_current_scene, Megabytes(4));

    enum game_scene current_scene = GSC_NONE;
    void *current_scene_handle = NULL;
    g_next_scene = GSC_PLAY;

    b32 quit = false;

    while (!quit) {

	// Manage scene transitions
	if (g_next_scene != GSC_NONE) {
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
	    struct game_state *game_state = (struct game_state *)current_scene_handle;

	    if (game_state->vars.is_tweakui_shown) {
		PollInputEvents();
		quit = WindowShouldClose();
		if (IsKeyReleased(KEY_P)) {
		    game_state->vars.is_tweakui_shown = false;
		}
	    } else {
		// Update game
		f64 new_time = GetTime();
		f64 frame_time = new_time - game_state->current_time;
		f64 total_update_time = 0;
		game_state->current_time = new_time;

		if (frame_time > 1) {
		    frame_time = game_state->dt;
		}

		u32 physics_iterations = 0;
		game_state->accumulator += frame_time;
		while (game_state->accumulator > 0) {
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

		    f64 update_time = GetTime();
		    game_play_update(game_state, &input);
		    total_update_time += GetTime() - update_time;

		    game_state->accumulator -= game_state->dt;
		    game_state->time += game_state->dt;

		    physics_iterations++;
		    game_state->physics_updates++;
		}

		game_state->frame_updates++;

		if (physics_iterations > 0) {
		    layers_debug_queue_draw_text(
			game_state->layer_debug,
			TextFormat("Update time: %0.6f", total_update_time / physics_iterations));
		    layers_debug_queue_draw_text(game_state->layer_debug,
						 TextFormat("Physics/frame: %2d",
							    physics_iterations));
		    layers_debug_queue_draw_text(game_state->layer_debug,
						 TextFormat("Physics: %12d",
							    game_state->physics_updates));
		    layers_debug_queue_draw_text(game_state->layer_debug,
						 TextFormat("Frame: %14d",
							    game_state->frame_updates));
		    layers_debug_queue_draw_text(game_state->layer_debug,
						 TextFormat("FPS: %3.0f", 1 / frame_time));
		}
	    }

	    game_play_draw(game_state);
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

void *handle_scene_creation(struct game_config config,
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

void handle_scene_destruction(enum game_scene current_scene, void *handle)
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
