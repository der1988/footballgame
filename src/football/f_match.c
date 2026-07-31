#include "entities/entities.h"
#include "entities/e_ball.h"
#include "events/events.h"
#include "football/football.h"
#include "math_ext/math_ext.h"
#include "log.h"
#include "game/game.h"
#include "entities/e_camera.h"
#include <stdio.h>

const char *pitch_side_to_string[PSI_COUNT] = {"north", "south"};
const char *team_side_to_string[TS_COUNT] = {"home", "away"};
const char *save_direction_to_string[SD_COUNT] = {"left", "right"};
const char *save_height_to_string[SH_COUNT] = {"high", "low"};

const char *match_state_to_string[MS_COUNT] = {"kickoff",
											   "half_time",
											   "extra_time",
											   "penalties",
											   "play",
											   "goal",
											   "throw_in",
											   "goal_kick",
											   "corner_kick",
											   "penalty_kick",
											   "free_kick"};

static struct player_characteristics characteristics_for_position(u8 idx)
{
	if (idx <= 3)
		return (struct player_characteristics){
			.speed = 4, .acceleration = 3, .strength = 4, .weight = 4}; // Defenders
	if (idx <= 6)
		return (struct player_characteristics){
			.speed = 3, .acceleration = 3, .strength = 3, .weight = 3}; // Midfielders
	return (struct player_characteristics){
		.speed = 4, .acceleration = 4, .strength = 2, .weight = 2}; // Forwards
}

void football_match_change_state(struct game_state *game_state, enum match_state new_state)
{
	game_state->vars.match.state = new_state;
	GLOGF_INFO(LS_GAME, "match state changed to: %s", match_state_to_string[new_state]);
	switch (new_state) {
	case MS_KICKOFF:
	case MS_THROW_IN:
	case MS_GOAL_KICK:
	case MS_CORNER_KICK:
	default:
		break;
	}
}

static void on_event(enum event_type event_type, void *context)
{
	struct game_state *game_state = context;

	switch (event_type) {
	case EVT_GOAL_SCORED:
		football_match_change_state(game_state, MS_GOAL);
		break;
	case EVT_BALL_OOB_NORTH:
	case EVT_BALL_OOB_SOUTH:
	case EVT_BALL_OOB_EAST:
	case EVT_BALL_OOB_WEST:
		// TODO: quick hack to put the ball back in play
		entity_ball_set_position_and_velocity(game_state->vars.ball.entity,
											  V2_TO_V3(game_state->vars.pitch.midfield_spot, 30),
											  Vector3Zero());
		break;
	case EVT_CONSTANTS_CHANGED:
	case EVT_COUNT:
		break;
	}
}

void football_match_setup(struct game_state *game_state)
{
	game_state->vars.teams[0] =
		(struct team){(struct team_info){.name = "New Team"},
					  (struct team_play_context){.pitch_side = PSI_NORTH, .formation = TF_433}};

	game_state->vars.teams[1] =
		(struct team){(struct team_info){.name = "Muppets"},
					  (struct team_play_context){.pitch_side = PSI_SOUTH, .formation = TF_433}};

	// Team 1
	for (usize i = 0; i < 10; ++i) {
		v2 pos = football_tactics_get_team_formation_position(TF_433, i)->position[TM_DEFAULT];
		pos.y = 1 - pos.y;
		entity_player_create(
			(struct player_config){.position = football_pitch_position_normalized_to_abs(
									   game_state->vars.pitch.field, pos),
								   .team_side = TS_HOME,
								   .team_formation_position_idx = i,
								   .characteristics = characteristics_for_position(i)},
			game_state);
	}

	// Team 2
	for (usize i = 0; i < 10; ++i) {
		v2 pos = football_tactics_get_team_formation_position(TF_433, i)->position[TM_DEFAULT];
		entity_player_create(
			(struct player_config){.position = football_pitch_position_normalized_to_abs(
									   game_state->vars.pitch.field, pos),
								   .team_side = TS_AWAY,
								   .team_formation_position_idx = i,
								   .characteristics = characteristics_for_position(i)},
			game_state);
	}

	entity_player_create((struct player_config){.is_goalie = true,
												.position =
													game_state->vars.pitch.goal[PSI_NORTH].center,
												.team_side = TS_HOME},
						 game_state);
	entity_player_create((struct player_config){.is_goalie = true,
												.position =
													game_state->vars.pitch.goal[PSI_SOUTH].center,
												.team_side = TS_AWAY},
						 game_state);

	struct entity *ball_entity = entity_ball_create(
		(struct ball_config){.position = game_state->vars.pitch.midfield_spot}, game_state);
	game_state->vars.ball.entity = ball_entity;

	struct entity *camera_entity = entity_camera_create(game_state);
	struct component_camera *camera = &camera_entity->camera;
	struct component_target *camera_target = &camera_entity->target;
	camera->camera = CAM_MAIN;
	camera_target->entity_id = ball_entity->header.id;
	game_state->vars.camera_entity = camera_entity;

	game_state->camera.zoom = 1;
	game_state->camera.rotation = 0;
	// game_state->camera.offset = (Vector2){(f32)game_state->config.screen_width / 2,
	//                                       (f32)game_state->config.screen_height / 2};
	game_state->virtual_camera.zoom = 1;
	game_state->virtual_camera.rotation = 0;
	game_state->virtual_camera.offset = (v2){(f32)game_state->config.virtual_screen_width / 2,
											 (f32)game_state->config.virtual_screen_height / 2};

	game_state->human_players[0] =
		(struct human_player){.enabled = true,
							  .controlled_player = {.entity_id = EID_NOT_VALID},
							  .team_side = TS_HOME,
							  .game_controller_id = 0};
	// game_state->human_players[1] =
	// 	(struct human_player){.enabled = false,
	// 						  .controlled_player = {.entity_id = EID_NOT_VALID},
	// 						  .team_side = TS_AWAY,
	// 						  .game_controller_id = 1};

	events_router_add_listener(game_state->events_router,
							   on_event,
							   5,
							   EVT_GOAL_SCORED,
							   EVT_BALL_OOB_NORTH,
							   EVT_BALL_OOB_SOUTH,
							   EVT_BALL_OOB_EAST,
							   EVT_BALL_OOB_WEST);

	events_router_add_listener(
		game_state->events_router, entity_player_on_constants_changed, 1, EVT_CONSTANTS_CHANGED);

	football_match_change_state(game_state, MS_PLAY);
}
