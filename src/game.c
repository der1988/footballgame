#include "game.h"
#include "assets.h"
#include "components.h"
#include "entities.h"
#include "football.h"
#include "input.h"
#include "layers.h"
#include "memory.h"
#include "raygui/raygui.h"
#include "systems.h"
#include "tweakui.h"
#include "types.h"
#include "log.h"
#include <assert.h>
#include <raylib.h>
#include <raymath.h>
#include "constants.h"

static struct goal setup_goal(entity_id_t id,
			      enum pitch_side side,
			      v2 center,
			      u16 width,
			      u16 height,
			      u16 penalty_box_width,
			      u16 penalty_box_height)
{
    struct goal goal = (struct goal){
	.id = id,
	.center = center,
	.penalty_box = (rect_t){.x = 165,
				.y = center.y - (side == PSI_NORTH ? 0 : penalty_box_height),
				.width = penalty_box_width,
				.height = penalty_box_height},
	.penalty_spot = {.x = center.x,
			 .y = center.y + 11 * PIXELS_PER_METER * (side == PSI_NORTH ? 1 : -1)},
	.goal_quad_points = {
	    {.x = center.x - width * 2, .y = 0, .z = center.y},
	    {.x = center.x - width * 2, .y = height, .z = center.y},
	    {.x = center.x + width * 2, .y = height, .z = center.y},
	    {.x = center.x + width * 2, .y = 0, .z = center.y},
	}};

    return goal;
}

struct game_state *game_init(struct game_config game_config)
{
    GLOGF_INFO(LS_GAME, "size of a single game entity: %lu bytes", sizeof(struct entity));
    memory_arena_handle_t *memory_arena_lifetime_game =
	memory_arena_create("game", game_config.memory, Megabytes(1));

    struct game_state *game_state =
	memory_arena_push(memory_arena_lifetime_game, sizeof(struct game_state));
    game_state->config = game_config;
    game_state->config.players_count = 1;

    // pitch in reality is 105m long
    // pitch in game is 1080 pixels long
    // 1m is ~10 pixels in game
    // gravity is -9.81m/s^2 in reality, so -98.1 pixels/s^2 in game
    game_state->constants =
	(struct game_constants){.fixed_timestep = 1 / 480.0,
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
				.ball_run_force = 30,
				.ball_control_force = 30,
				// This will translate to instant velocity, so 144km/h
				.ball_shot_force = 40 * PIXELS_PER_METER,
				.ball_shot_height = 0.25f,
				.ball_pass_force = 30 * PIXELS_PER_METER,
				.ball_long_pass_force = 24 * PIXELS_PER_METER,
				.ball_long_pass_height = 1.0f,
				.ball_after_touch_delay_time = 0.1f,
				.ball_after_touch_max_time = 0.6f,
				.ball_after_touch_force = 150.0f,
				// 36m/s seems fair, that would be 129km/h
				.ball_max_velocity = 50 * PIXELS_PER_METER,
				.ball_lookaheads = 10,
				.ball_lookahead_dts = 96,
				.crossbar_height = 27,
				.player_movement_force = 27000,
				.player_movement_drag = 4166,
				.player_control_max_velocity = 6.6f * PIXELS_PER_METER,
				// Fastest player should reach ~10.5m/s, slowest 7.8m/s
				.player_run_max_velocity = 10.5f * PIXELS_PER_METER,
				.player_control_radius = 6,
				.player_control_height = 19,
				.player_run_control_max_angle = 45,
				// Elite goalie has 150ms reaction time
				.goalie_dive_reaction_time = 0.150f,
				.goalie_dive_force = 8500,
				// TODO: This parameter should depend on goalie's quality
				.goalie_dive_max_velocity = 5 * PIXELS_PER_METER,
				// Minimum speed for goalie to dive is 54km/h
				.goalie_dive_min_ball_velocity = 15 * PIXELS_PER_METER,
				.ai_enabled = true,
				.shoot_aid_no_inertia = true,
				.kick_charge_reset_time = 1.5f};

    // Partition game memory
    game_state->memory_by_lifetime[MAL_GAME] = memory_arena_lifetime_game;
    game_state->memory_by_lifetime[MAL_LEVEL] =
	memory_arena_create("level", game_config.memory + Megabytes(1), Megabytes(1));
    game_state->memory_by_lifetime[MAL_UPDATE] =
	memory_arena_create("frame", game_config.memory + Megabytes(2), Megabytes(1));
    game_state->memory_by_lifetime[MAL_DRAW] =
	memory_arena_create("frame", game_config.memory + Megabytes(3), Megabytes(1));

    // Initialise entities repository
    game_state->entity_repository =
	entities_repository_init(game_state->memory_by_lifetime[MAL_GAME]);

    // Initialise caches
    game_state->texture_cache =
	assets_texture_cache_create(game_state->memory_by_lifetime[MAL_GAME]);
    game_state->sprite_frame_cache =
	assets_sprite_frame_cache_create(game_state->memory_by_lifetime[MAL_GAME]);
    game_state->animation_cache =
	assets_animation_cache_create(game_state->memory_by_lifetime[MAL_GAME]);

    // Load pitch
    struct assets_texture *texture =
	assets_texture_load(game_state->texture_cache, "res/pitch.png");
    assert(texture);
    game_state->pitch_texture_id = texture->id;

    // Load main spritesheet
    texture = assets_texture_load(game_state->texture_cache, "res/spritesheets/footballgame.png");
    assert(texture);
    i32 loaded = assets_sprite_frames_load_from_json(
	game_state->sprite_frame_cache, texture->id, "res/spritesheets/footballgame.json");
    assert(loaded != -1);

    // Load other spritesheet
    texture = assets_texture_load(game_state->texture_cache, "res/spritesheets/other.png");
    assert(texture);
    loaded = assets_sprite_frames_load_from_json(
	game_state->sprite_frame_cache, texture->id, "res/spritesheets/other.json");
    assert(loaded != -1);

    // Load Animations
    assets_animations_load_from_json(game_state->animation_cache,
				     game_state->sprite_frame_cache,
				     "res/animations/animations.json");

    game_state->tweakui = tweakui_create(game_state->memory_by_lifetime[MAL_GAME]);

    // Layers
    game_state->layer_debug = layers_debug_create(game_state);

#ifndef RUNNING_TEST
    game_state->vars.is_debug_log_shown = true;

    game_state->config.players_count = 1;

    game_state->vars.pitch.field = (rect_t){.x = 33, .y = 79, .width = 640, .height = 360 * 3 - 4};
    game_state->vars.pitch.goal_width = 79;
    game_state->vars.pitch.goal_height = 27;
    game_state->vars.pitch.penalty_box_width = 374;
    game_state->vars.pitch.penalty_box_height = 167;
    game_state->vars.pitch.midfield_spot =
	(v2){.x = game_state->vars.pitch.field.x + game_state->vars.pitch.field.width * 0.5f,
	     .y = game_state->vars.pitch.field.y + game_state->vars.pitch.field.height * 0.5f};

    // Set up goals
    for (usize i = 0; i < PSI_COUNT; ++i) {

	v2 goal_center =
	    (v2){.x = game_state->vars.pitch.field.x + game_state->vars.pitch.field.width * 0.5f,
		 .y = game_state->vars.pitch.field.y + game_state->vars.pitch.field.height * i};

	struct entity *goal_entity = entities_goal_create(
	    (struct goal_config){.side = i, .center = goal_center}, game_state);

	game_state->vars.pitch.goal[i] = setup_goal(goal_entity->header.id,
						    i,
						    goal_center,
						    game_state->vars.pitch.goal_width,
						    game_state->vars.pitch.goal_height,
						    game_state->vars.pitch.penalty_box_width,
						    game_state->vars.pitch.penalty_box_height);
    }

    struct entity *player_entity = entities_player_create(
	(struct player_config){
	    .position = football_pitch_position_rel_to_abs(
		game_state->vars.pitch.field,
		football_tactics_get_team_formation_position(TF_433, 0)->position[TM_DEFAULT]),
	    .side = PSI_NORTH,
	    .team_formation_position_idx = 0},
	game_state);

    //    for (usize i = 1; i < 10; ++i) {
    // entities_player_create(
    //     (struct player_config){
    // 	.position = football_pitch_position_rel_to_abs(
    // 	    game_state->vars.pitch.field,
    // 	    football_tactics_get_team_formation_position(TF_433, i)->position[TM_DEFAULT]),
    // 	.side = PSI_NORTH,
    // 	.team_formation_position_idx = i},
    //     game_state);
    //    }

    entities_player_create((struct player_config){.is_goalie = true,
						  .position =
						      game_state->vars.pitch.goal[PSI_NORTH].center,
						  .side = PSI_NORTH},
			   game_state);
    entities_player_create((struct player_config){.is_goalie = true,
						  .position =
						      game_state->vars.pitch.goal[PSI_SOUTH].center,
						  .side = PSI_SOUTH},
			   game_state);

    struct entity *ball_entity = entities_ball_create(
	(struct ball_config){.position = game_state->vars.pitch.midfield_spot}, game_state);
    game_state->vars.ball_entity_id = ball_entity->header.id;

    struct entity *camera_entity = entities_camera_create(game_state);
    struct component_camera *camera = &camera_entity->camera;
    struct component_target *camera_target = &camera_entity->target;
    camera->camera = CAM_MAIN;
    camera_target->entity_id = ball_entity->header.id;
    game_state->vars.camera_entity_id = camera_entity->header.id;

    game_state->camera.zoom = 1;
    game_state->camera.rotation = 0;
    // game_state->camera.offset = (Vector2){(f32)game_state->config.screen_width / 2,
    //                                       (f32)game_state->config.screen_height / 2};
    game_state->virtual_camera.zoom = 1;
    game_state->virtual_camera.rotation = 0;
    game_state->virtual_camera.offset = (v2){(f32)game_state->config.virtual_screen_width / 2,
					     (f32)game_state->config.virtual_screen_height / 2};

    football_match_change_state(game_state, MS_PLAY);
    game_state->vars.p1 = (struct human_player){.player_entity_id = player_entity->header.id};
#endif

    game_state->render_texture = LoadRenderTexture((f32)game_state->config.virtual_screen_width,
						   (f32)game_state->config.virtual_screen_height);

    game_state->initialised = true;
    GLOG_INFO(LS_GAME, "game initialisation complete");

    return game_state;
}

void game_destroy(struct game_state *game_state)
{
    assert(game_state->initialised);

    UnloadRenderTexture(game_state->render_texture);

    layers_debug_destroy(game_state->layer_debug);

    tweakui_destroy(game_state->tweakui);

    assets_animation_cache_destroy(game_state->animation_cache);
    assets_sprite_frame_cache_destroy(game_state->sprite_frame_cache);
    assets_texture_cache_destroy(game_state->texture_cache);

    entities_repository_destroy(game_state->entity_repository);

    memory_arena_destroy(game_state->memory_by_lifetime[MAL_GAME]);
    memory_arena_destroy(game_state->memory_by_lifetime[MAL_LEVEL]);
    memory_arena_destroy(game_state->memory_by_lifetime[MAL_UPDATE]);

    game_state->initialised = false;
    GLOG_INFO(LS_GAME, "game destruction complete");
}

void game_update(struct game_state *game_state, struct input *input)
{
    assert(game_state->initialised);

    // These should be reset every update
    memory_arena_reset(game_state->memory_by_lifetime[MAL_UPDATE]);
    layers_debug_reset(game_state->layer_debug);

    // Slow motion
    game_state->vars.slow_motion = input->buttons[IBT_BUTTON_TRIGGER_LEFT].down;
    if (game_state->vars.slow_motion) {
	game_state->time -= game_state->dt;
	game_state->dt = game_state->dt * 0.1;
	game_state->time += game_state->dt;
	game_state->vars.slow_motion = true;
    }

    systems_controls(game_state, input);
    systems_goalie(game_state);
    systems_player(game_state, input);
    systems_player_on_the_ball(game_state, input);
    systems_player_selection(game_state, input);
    systems_ball(game_state, input);
    systems_goal(game_state);
    systems_action(game_state);
    systems_camera(game_state);
}

void game_draw(struct game_state *game_state)
{
    assert(game_state->initialised);

    memory_arena_reset(game_state->memory_by_lifetime[MAL_DRAW]);

    struct component_transform *camera_transform =
	&entities_entity_get(game_state->entity_repository, game_state->vars.camera_entity_id)
	     ->transform;

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

    texture_t pitch_texture =
	assets_texture_get_by_id(game_state->texture_cache, game_state->pitch_texture_id)->texture;

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

    v2 origin = {0.0f, 0.0f};

    BeginMode2D(game_state->camera);
    DrawTexturePro(game_state->render_texture.texture, source_rec, dest_rec, origin, 0.0f, WHITE);
    EndMode2D();

    layers_hud_draw(game_state);

    if (game_state->vars.is_debug_log_shown) {
	layers_debug_draw(game_state->layer_debug, game_state);
    }

    // Tweak UI
    if (game_state->vars.is_tweakui_shown) {
	tweakui_draw(game_state->tweakui, game_state);
    }

    EndDrawing();
}
