#include "entity.h"
#include "entity_ball.h"
#include "entity_camera.h"
#include "entity_player.h"
#include "events_router.h"
#include "football.h"
#include "game.h"
#include "gameplay.h"
#include "gameplay_match.h"
#include "gameplay_sandbox.h"
#include "log.h"
#include "macro.h"

const char *gameplay_mode_string[GPM_COUNT] = {"match", "sandbox"};

static const struct gameplay_vtable *gameplay_vtables[GPM_COUNT] = {
	[GPM_MATCH] = &gameplay_match_vtable,
	[GPM_SANDBOX] = &gameplay_sandbox_vtable,
};

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

// A keeper is rated like anybody else: big and strong, no quicker than a midfielder.
static struct player_characteristics characteristics_for_goalie(void)
{
	return (struct player_characteristics){
		.speed = 3, .acceleration = 3, .strength = 4, .weight = 4};
}

// Translates an event into a trigger and fires it. What the trigger means is
// the mode's transition table to decide, never this function.
static void on_event(enum event_type event_type, void *context)
{
	struct game_state *game_state = context;

	switch (event_type) {
	case EVT_GOAL_SCORED:
		gameplay_fire(FT_GOAL_SCORED, game_state);
		break;
	case EVT_BALL_OOB_NORTH:
	case EVT_BALL_OOB_SOUTH:
	case EVT_BALL_OOB_EAST:
	case EVT_BALL_OOB_WEST:
		gameplay_fire(football_classify_ball_out(event_type, game_state), game_state);
		break;
	case EVT_CONSTANTS_CHANGED:
	case EVT_COUNT:
		break;
	}
}

void gameplay_setup_common(u8 outfield_players_per_team, struct game_state *game_state)
{
	game_state->vars.teams[0] =
		(struct team){(struct team_info){.name = "New Team"},
					  (struct team_play_context){.pitch_side = PSI_NORTH, .formation = TF_433}};

	game_state->vars.teams[1] =
		(struct team){(struct team_info){.name = "Muppets"},
					  (struct team_play_context){.pitch_side = PSI_SOUTH, .formation = TF_433}};

	// Team 1
	for (usize i = 0; i < outfield_players_per_team; ++i) {
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
	for (usize i = 0; i < outfield_players_per_team; ++i) {
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
												.team_side = TS_HOME,
												.characteristics = characteristics_for_goalie()},
						 game_state);
	entity_player_create((struct player_config){.is_goalie = true,
												.position =
													game_state->vars.pitch.goal[PSI_SOUTH].center,
												.team_side = TS_AWAY,
												.characteristics = characteristics_for_goalie()},
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
	game_state->virtual_camera.offset =
		(v2){(f32)game_state->config.virtual_screen_width / 2 + VIRTUAL_SCREEN_BLEED,
			 (f32)game_state->config.virtual_screen_height / 2 + VIRTUAL_SCREEN_BLEED};

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
}

void gameplay_init(enum gameplay_mode mode, struct game_state *game_state)
{
	ASSERT(mode < GPM_COUNT);

	struct gameplay *gameplay = &game_state->gameplay;

	const struct gameplay_vtable *vtable = gameplay_vtables[mode];
	ASSERT(vtable);

	// Written before anything can read the union it tags.
	gameplay->mode = mode;

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

	vtable->setup(game_state);

	// Last: init runs on_enter, which needs the entities setup has just spawned.
	state_machine_football_init(
		&gameplay->state_machine, gameplay, vtable->transitions, FS_PLAY, game_state);

	GLOGF_INFO(LS_GAME, "gameplay initialised in %s mode", vtable->name);
}

void gameplay_fire(enum state_machine_football_trigger trigger, struct game_state *game_state)
{
	struct gameplay *gameplay = &game_state->gameplay;
	ASSERT(gameplay->state_machine.current_state != FS_NONE);
	state_machine_football_fire(&gameplay->state_machine, trigger, game_state);
}

// Answers "may this state advance yet?" — the deferred exit conditions the
// transition table has no way to express.
void gameplay_update(struct game_state *game_state)
{
	struct gameplay *gameplay = &game_state->gameplay;
	ASSERT(gameplay->state_machine.current_state != FS_NONE);

	struct state_machine_football *state_machine = &gameplay->state_machine;

	switch (state_machine->current_state) {
	case FS_GOAL:
		if (HAS_TIME_PASSED(game_state->time,
							state_machine->state_data.goal.start_time,
							gameplay->config.goal_celebration_time)) {
			gameplay_fire(FT_CELEBRATION_ENDED, game_state);
		}
		break;
	case FS_KICKOFF:
	case FS_THROW_IN:
	case FS_CORNER_KICK:
	case FS_GOAL_KICK:
		// Nothing to wait for yet; on_enter has already placed the ball.
		gameplay_fire(FT_PLAY_RESUMED, game_state);
		break;
	case FS_NONE:
	case FS_HALF_TIME:
	case FS_EXTRA_TIME:
	case FS_PENALTIES:
	case FS_PLAY:
	case FS_PENALTY_KICK:
	case FS_FREE_KICK:
	case FS_COUNT:
		break;
	}
}
