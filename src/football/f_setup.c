#include "entities.h"
#include "football.h"
#include "game.h"

static struct goal setup_goal(entity_id_t id,
							  enum pitch_side side,
							  v2 center,
							  u16 width,
							  u16 height,
							  u16 penalty_box_width,
							  u16 penalty_box_height);

void football_setup(struct game_state *game_state)
{
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

	game_state->vars.teams[0] =
		(struct team){(struct team_info){.name = "New Team"},
					  (struct team_play_context){.pitch_side = PSI_NORTH, .formation = TF_433}};

	game_state->vars.teams[1] =
		(struct team){(struct team_info){.name = "Muppets"},
					  (struct team_play_context){.pitch_side = PSI_SOUTH, .formation = TF_433}};

	struct entity *player_entity = entities_player_create(
		(struct player_config){
			.position = football_pitch_position_normalized_to_abs(
				game_state->vars.pitch.field,
				football_tactics_get_team_formation_position(TF_433, 0)->position[TM_DEFAULT]),
			.team_side = TS_HOME,
			.team_formation_position_idx = 0},
		game_state);

	// Team 1
	for (usize i = 1; i < 10; ++i) {
		v2 pos = football_tactics_get_team_formation_position(TF_433, i)->position[TM_DEFAULT];
		pos.y = 1 - pos.y;
		entities_player_create((struct player_config){.position =
														  football_pitch_position_normalized_to_abs(
															  game_state->vars.pitch.field, pos),
													  .team_side = TS_HOME,
													  .team_formation_position_idx = i},
							   game_state);
	}

	// Team 2
	for (usize i = 0; i < 10; ++i) {
		v2 pos = football_tactics_get_team_formation_position(TF_433, i)->position[TM_DEFAULT];
		entities_player_create((struct player_config){.position =
														  football_pitch_position_normalized_to_abs(
															  game_state->vars.pitch.field, pos),
													  .team_side = TS_AWAY,
													  .team_formation_position_idx = i},
							   game_state);
	}

	entities_player_create((struct player_config){.is_goalie = true,
												  .position =
													  game_state->vars.pitch.goal[PSI_NORTH].center,
												  .team_side = TS_HOME},
						   game_state);
	entities_player_create((struct player_config){.is_goalie = true,
												  .position =
													  game_state->vars.pitch.goal[PSI_SOUTH].center,
												  .team_side = TS_AWAY},
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
	game_state->human_players[0] = (struct human_player){
		.controlled_player_entity_id = player_entity->header.id, .team_side = TS_HOME};
}

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
