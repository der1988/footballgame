#include "entities.h"
#include "events/events.h"
#include "football.h"
#include "log.h"
#include "memory.h"
#include "systems.h"
#include "input.h"
#include "game.h"
#include "gfx.h"
#include "time_machine/time_machine.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include "platform/platform.h"

void update(struct game_state *game_state);
void draw(struct game_state *game_state);

struct game_state *game_play_init(struct game_config game_config,
								  struct assets assets,
								  memory_arena_handle_t *memory_arena)
{
	GLOGF_INFO(LS_GAME, "size of a single game entity: %lu bytes", sizeof(struct entity));

	fg_time_t fixed_timestep = 1 / 480.0;

	// Push game state onto main memory arena
	struct game_state *game_state = memory_arena_push(memory_arena, sizeof(struct game_state));
	assert(game_state);
	game_state->current_time = GetTime();
	game_state->accumulator = 0.0;
	game_state->dt = fixed_timestep;
	game_state->config = game_config;
	game_state->assets = assets;

	// Push some memory for our own arenas onto main memory arena
	void *game_memory = memory_arena_push(memory_arena, Megabytes(97));
	assert(game_memory);
	game_state->memory_by_lifetime[MAL_LEVEL] =
		memory_arena_create_default("level", game_memory, Megabytes(1));
	game_state->memory_by_lifetime[MAL_UPDATE] =
		memory_arena_create_default("update", game_memory + Megabytes(1), Megabytes(1));
	game_state->memory_by_lifetime[MAL_DRAW] =
		memory_arena_create_default("draw", game_memory + Megabytes(2), Megabytes(1));
	game_state->memory_by_lifetime[MAL_TIME_MACHINE] =
		memory_arena_create_default("time_machine", game_memory + Megabytes(3), Megabytes(94));

	// Initialise events router
	game_state->events_router = events_router_create(game_state->memory_by_lifetime[MAL_LEVEL]);

	// Prepare GFX
	gfx_player_prepare(game_state);
	gfx_goalie_prepare(game_state);

	// pitch in reality is 105m long
	// pitch in game is 1080 pixels long
	// 1m is ~10 pixels in game
	// gravity is -9.81m/s^2 in reality, so -98.1 pixels/s^2 in game
	game_state->constants =
		(struct game_constants){.fixed_timestep = fixed_timestep,
								.render_debug = false,
								.input_stick_dead_zone = 0.05f,
								// Coefficient of restitution
								.ball_cr = 0.5f,
								.ball_cr_against_post = 0.7f,
								.pixels_per_meter = PIXELS_PER_METER,
								.air_density = 1.21f,
								.ball_cross_sectional_area = 0.038f,
								.ball_drag_coefficient = 0.25f,
								.gravity_force = {.y = -9.81f * PIXELS_PER_METER},
								.ball_friction_on_pitch = 0.5f,
								.ball_run_force = 40,
								.ball_control_force = 30,
								// This will translate to instant velocity, so 144km/h
								.ball_shot_force = 40 * PIXELS_PER_METER,
								.ball_shot_height = 0.25f,
								.ball_pass_force = 30 * PIXELS_PER_METER,
								.ball_long_pass_force = 32 * PIXELS_PER_METER,
								.ball_long_pass_height = 0.54f,
								.ball_tackle_force = 20 * PIXELS_PER_METER,
								.ball_header_force = 25 * PIXELS_PER_METER,
								// 36m/s seems fair, that would be 129km/h
								.ball_max_velocity = 36 * PIXELS_PER_METER,
								.ball_max_velocity_to_gain_possession = 20 * PIXELS_PER_METER,
								.ball_lookaheads = 5,
								.ball_lookahead_dts = 96,
								// Crossbar is at 2.44m height
								.crossbar_height = 27,

								.player_movement_force = 27000,
								.player_movement_drag = 4166,
								.player_control_max_velocity = 6.6f * PIXELS_PER_METER,
								// Fastest player should reach ~10.5m/s, slowest 7.8m/s
								.player_run_max_velocity = 10.5f * PIXELS_PER_METER,
								.player_control_radius = 6,
								.player_control_height = 19,
								// Ronaldo's record is 2.93m but average is just above crossbar
								.player_header_min_height = 22,
								.player_header_max_height = 28,
								.player_run_control_max_angle = 45,

								// Elite goalie has 150ms reaction time
								// 75ms seems more fun though
								.goalie_dive_reaction_time = 0.075f,
								.goalie_dive_force = 8500,
								// TODO: This parameter should depend on goalie's quality
								.goalie_dive_max_velocity = 5 * PIXELS_PER_METER,
								// Minimum speed for goalie to dive is 54km/h
								.goalie_dive_min_ball_velocity = 15 * PIXELS_PER_METER,
								// Average max reach for a gk is 3.2m
								.goalie_reach_max_height = 35.4f,
								.ai_enabled = true,
								.shoot_aid_no_inertia = true,
								.kick_charge_reset_time = 1.5f};

	// Initialise entities repository
	game_state->entity_repository =
		entity_repository_init(game_state->memory_by_lifetime[MAL_LEVEL]);

	// Initialise time machine
	game_state->time_machine =
		time_machine_create(game_state->memory_by_lifetime[MAL_TIME_MACHINE]);

	// Initialise input manager
	game_state->input_manager = input_manager_create(game_state->memory_by_lifetime[MAL_LEVEL]);

	// Get pitch texture id
	game_state->pitch_texture =
		asset_texture_get_by_name(game_state->assets.texture_cache, "res/pitch.png");
	assert(game_state->pitch_texture);

	game_state->tweakui = tweakui_create(game_state->memory_by_lifetime[MAL_LEVEL]);

	// Layers
	game_state->layer_debug = layer_debug_create(game_state);
	game_state->vars.is_debug_log_shown = true;

	game_state->render_texture = LoadRenderTexture((f32)game_state->config.virtual_screen_width,
												   (f32)game_state->config.virtual_screen_height);

#ifndef RUNNING_TEST

	football_setup(game_state);

#endif

	game_state->initialised = true;
	GLOG_INFO(LS_GAME, "gameplay initialisation complete");

	return game_state;
}

void game_play_destroy(struct game_state *game_state)
{
	assert(game_state->initialised);

	UnloadRenderTexture(game_state->render_texture);

	layer_debug_destroy(game_state->layer_debug);

	tweakui_destroy(game_state->tweakui);

	input_manager_destroy(game_state->input_manager);

	time_machine_destroy(game_state->time_machine);

	entity_repository_destroy(game_state->entity_repository);

	events_router_destroy(game_state->events_router);

	memory_arena_destroy(game_state->memory_by_lifetime[MAL_DRAW]);
	memory_arena_destroy(game_state->memory_by_lifetime[MAL_UPDATE]);
	memory_arena_destroy(game_state->memory_by_lifetime[MAL_LEVEL]);

	game_state->initialised = false;
	GLOG_INFO(LS_GAME, "gameplay destruction complete");
}

b32 game_play_update(struct game_state *game_state)
{
	assert(game_state->initialised);

	b32 quit = false;

	game_state->dt = game_state->constants.fixed_timestep;

	switch (game_state->vars.mode) {
	case GM_TICK: {
		// Game tick
		f64 new_time = GetTime();
		f64 frame_time = new_time - game_state->current_time;
		f64 total_update_time = 0;
		game_state->current_time = new_time;

		// If frame time is too big we were probably spending time in another mode
		if (frame_time > 0.5f) {
			frame_time = game_state->dt;
		}

		u32 physics_iterations = 0;
		game_state->accumulator += frame_time;
		while (game_state->accumulator > 0) {
			PollInputEvents();
			quit |= WindowShouldClose();

			input_manager_update(game_state->input_manager, game_state);

			game_state->time += game_state->dt;

			// Update game logic
			f64 update_time = GetTime();
			update(game_state);
			total_update_time += GetTime() - update_time;

			game_state->accumulator -= game_state->dt;

			physics_iterations++;
			game_state->physics_updates++;
		}

		game_state->frame_updates++;

		if (physics_iterations > 0) {
			layer_debug_queue_draw_text(game_state->layer_debug,
										TextFormat("Update time: %0.6f",
												   total_update_time / physics_iterations));
			layer_debug_queue_draw_text(game_state->layer_debug,
										TextFormat("Physics/frame: %2d", physics_iterations));
			layer_debug_queue_draw_text(game_state->layer_debug,
										TextFormat("Physics: %12d", game_state->physics_updates));
			layer_debug_queue_draw_text(game_state->layer_debug,
										TextFormat("Frame: %14d", game_state->frame_updates));
			layer_debug_queue_draw_text(game_state->layer_debug,
										TextFormat("FPS: %3.0f", 1 / frame_time));
		}
	} break;
	case GM_RECORDER:
		PollInputEvents();
		quit = WindowShouldClose();

		static usize tick = 0;
		tick++;

		/* We need to reset this here because this is usually done in the update function but in recorder mode that is not called. */
		layer_debug_reset(game_state->layer_debug);

		input_manager_update(game_state->input_manager, game_state);

		f32 left_trigger = game_state->input_manager->game_controllers[0]
							   .state.current[GAME_CONTROL_RECORDER_REWIND]
							   .analogic.value;
		f32 right_trigger = game_state->input_manager->game_controllers[0]
								.state.current[GAME_CONTROL_RECORDER_FORWARD]
								.analogic.value;

		static fg_time_t last_interaction_time = 0;

		if (left_trigger > 0.01f) {
			last_interaction_time = GetTime();
			left_trigger = 1 - left_trigger;
			left_trigger *= 15;
			left_trigger += 1;
			u32 rewind_speed = (u32)left_trigger;
			if (tick % rewind_speed == 0) {
				time_machine_travel(game_state->time_machine, -1, game_state);
			}
		} else if (right_trigger > 0.1f) {
			last_interaction_time = GetTime();
			right_trigger = 1 - right_trigger;
			right_trigger *= 15;
			right_trigger += 1;
			u32 forward_speed = (u32)right_trigger;
			if (tick % forward_speed == 0) {
				time_machine_travel(game_state->time_machine, 1, game_state);
			}
		}

		// if (HAS_TIME_PASSED(GetTime(), last_interaction_time, 3.0f)) {
		// 	time_machine_set_resume_position(
		// 		game_state->time_machine, RP_LAST_RECORDED, game_state);
		// 	game_state->vars.mode = GM_TICK;
		// } else
		struct game_controller *gc = &game_state->input_manager->game_controllers[0];
		if (gc->state.current[GAME_CONTROL_RECORDER_RESUME].digital.pressed) {
			time_machine_set_resume_position(game_state->time_machine, RP_CURRENT, game_state);
			game_state->vars.mode = GM_TICK;
		}
		break;
	case GM_TWEAK_UI: {
		PollInputEvents();

		/* We need to reset this here because this is usually done in the update function but in tweak ui mode that is not called. */
		layer_debug_reset(game_state->layer_debug);

		quit = WindowShouldClose();
		if (IsKeyReleased(KEY_P)) {
			game_state->vars.mode = GM_TICK;
		}
	} break;
	}

	layer_debug_queue_draw_text(game_state->layer_debug,
								TextFormat("Match State: %s",
										   match_state_to_string[game_state->vars.match.state]));

	// Draw game
	draw(game_state);

	return quit;
}

void update(struct game_state *game_state)
{
	// These should be reset every update
	memory_arena_reset(game_state->memory_by_lifetime[MAL_UPDATE]);
	layer_debug_reset(game_state->layer_debug);

	// mode selection
	if (IsKeyReleased(KEY_P)) {
		game_state->vars.mode = GM_TWEAK_UI;
	}

	if (game_state->input_manager->game_controllers[0]
			.state.current[GAME_CONTROL_RECORDER_REWIND]
			.analogic.value > 0.1f) {
		game_state->vars.mode = GM_RECORDER;
	}

	// debug layer
	if (IsKeyReleased(KEY_C)) {
		game_state->vars.is_debug_log_shown = !game_state->vars.is_debug_log_shown;
	}

	systems_goalie(game_state);
	systems_player_collisions(game_state);
	systems_team_pressing(game_state);
	systems_player_control_human(game_state);
	systems_player_control_ai(game_state);
	systems_players_off_the_ball(game_state);
	systems_player_on_the_ball(game_state);
	systems_player_physics(game_state);
	systems_player_selection(game_state);
	systems_ball(game_state);
	systems_goal(game_state);
	systems_action(game_state);
	systems_camera(game_state);
	systems_time_machine(game_state);
}

void draw(struct game_state *game_state)
{
	assert(game_state->initialised);

	memory_arena_reset(game_state->memory_by_lifetime[MAL_DRAW]);

	struct component_transform *camera_transform = &game_state->vars.camera_entity->transform;

	// Round virtual space coordinates, keep decimals into screen space coordinates
	f32 virtual_ratio =
		(f32)game_state->config.screen_width / (f32)game_state->config.virtual_screen_width;

	game_state->virtual_camera.target.x = (u64)camera_transform->position.x;
	game_state->camera.target.x =
		camera_transform->position.x - game_state->virtual_camera.target.x;
	game_state->camera.target.x *= virtual_ratio;

	game_state->virtual_camera.target.y = (u64)camera_transform->position.y;
	game_state->camera.target.y =
		camera_transform->position.y - game_state->virtual_camera.target.y;
	game_state->camera.target.y *= virtual_ratio;

	texture_t pitch_texture = game_state->pitch_texture->texture;

	BeginTextureMode(game_state->render_texture);
	ClearBackground((Color){.r = 42, .g = 99, .b = 1, .a = 255});

	BeginMode2D(game_state->virtual_camera);

	f32 stretched_width = pitch_texture.width * 1.0f;
	f32 stretched_height = pitch_texture.height * 1.0f;
	DrawTexturePro(
		pitch_texture,
		(rect_t){.x = 0, .y = 0, .width = pitch_texture.width, .height = pitch_texture.height},
		(rect_t){.x = 0, .y = 0, .width = stretched_width, .height = stretched_height},
		Vector2Zero(),
		0,
		WHITE);

	systems_render(game_state);
	systems_render_hud(game_state);
	systems_render_debug(game_state);

	EndMode2D();

	EndTextureMode();

	BeginDrawing();
	ClearBackground(BLACK);

	// The target's height is flipped (in the source Rectangle), due to OpenGL reasons
	rect_t source_rec = {0.0f,
						 0.0f,
						 (float)game_state->render_texture.texture.width,
						 -(float)game_state->render_texture.texture.height};
	rect_t dest_rec = {-virtual_ratio,
					   -virtual_ratio,
					   game_state->config.screen_width + (virtual_ratio * 2),
					   game_state->config.screen_height + (virtual_ratio * 2)};

	v2 origin = Vector2Zero();

	BeginMode2D(game_state->camera);
	DrawTexturePro(game_state->render_texture.texture, source_rec, dest_rec, origin, 0.0f, WHITE);
	EndMode2D();

	layer_hud_draw(game_state);

	if (game_state->vars.is_debug_log_shown) {
		layer_debug_draw(game_state->layer_debug, game_state);
	}

	// Tweak UI
	if (game_state->vars.mode == GM_TWEAK_UI) {
		tweakui_draw(game_state->tweakui, game_state);
	}

	EndDrawing();
}
