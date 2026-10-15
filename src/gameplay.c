#include "entity.h"
#include "entity_ball.h"
#include "entity_camera.h"
#include "entity_player.h"
#include "entity_referee.h"
#include "events_router.h"
#include "football.h"
#include "game.h"
#include "gameplay.h"
#include "gameplay_match.h"
#include "gameplay_sandbox.h"
#include "log.h"
#include "macro.h"
#include <float.h>
#include <math.h>
#include <string.h>
#include "math_ext.h"
#include "input.h"
#include <raymath.h>

const char *gameplay_mode_string[GPM_COUNT] = {"match", "sandbox"};

static const struct gameplay_vtable *gameplay_vtables[GPM_COUNT] = {
	[GPM_MATCH] = &gameplay_match_vtable,
	[GPM_SANDBOX] = &gameplay_sandbox_vtable,
};

static struct player_characteristics characteristics_for_position(enum team_formation formation, u8 idx)
{
	enum player_role role = football_tactics_get_team_formation_position(formation, idx)->supported_roles[0];
	if (role == PR_CB || role == PR_SB || role == PR_WB)
		return (struct player_characteristics){
			.speed = 4, .acceleration = 3, .strength = 4, .weight = 4}; // Defenders
	if (role != PR_CF && role != PR_WF && role != PR_SS)
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
					  (struct team_play_context){.pitch_side = PSI_NORTH, .formation = game_state->config.home_formation}};

	game_state->vars.teams[1] =
		(struct team){(struct team_info){.name = "Muppets"},
					  (struct team_play_context){.pitch_side = PSI_SOUTH, .formation = game_state->config.away_formation}};

	const enum team_formation home = game_state->vars.teams[TS_HOME].play_context.formation;
	const enum team_formation away = game_state->vars.teams[TS_AWAY].play_context.formation;
	u8 home_count = outfield_players_per_team;
	u8 away_count = outfield_players_per_team;
	GLOGF_INFO(LS_GAME, "FORMATIONS home=%s away=%s outfield=%u/%u",
		team_formation_to_string[home], team_formation_to_string[away], home_count, away_count);

	// Team 1
	for (usize i = 0; i < home_count; ++i) {
		v2 pos = football_tactics_get_team_formation_position(home, i)->position[TM_DEFAULT];
		pos.y = 1 - pos.y;
		entity_player_create(
			(struct player_config){.position = football_pitch_position_normalized_to_abs(
									   game_state->vars.pitch.field, pos),
								   .team_side = TS_HOME,
								   .team_formation_position_idx = i,
								   .characteristics = characteristics_for_position(home, i)},
			game_state);
	}

	// Team 2
	for (usize i = 0; i < away_count; ++i) {
		v2 pos = football_tactics_get_team_formation_position(away, i)->position[TM_DEFAULT];
		entity_player_create(
			(struct player_config){.position = football_pitch_position_normalized_to_abs(
									   game_state->vars.pitch.field, pos),
								   .team_side = TS_AWAY,
								   .team_formation_position_idx = i,
								   .characteristics = characteristics_for_position(away, i)},
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
	entity_referee_create(game_state);

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
		&gameplay->state_machine, gameplay, vtable->transitions, FS_KICKOFF, game_state);

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
	case FS_FREE_KICK:
	case FS_PENALTY_KICK:
		if (gameplay->restart.prepared && (gameplay->restart.manual ? gameplay_update_restart_input(game_state) : game_state->time - gameplay->restart.start_time >= 2.0)) {
			struct entity *ball = game_state->vars.ball.entity;
			const enum pitch_side own = game_state->vars.teams[gameplay->restart.team].play_context.pitch_side;
			v2 target = game_state->vars.pitch.goal[own == PSI_NORTH ? PSI_SOUTH : PSI_NORTH].center;
			if (state_machine->current_state == FS_CORNER_KICK) target.y += own == PSI_NORTH ? -70 : 70;
			if (state_machine->current_state == FS_KICKOFF || state_machine->current_state == FS_THROW_IN)
				target = Vector2Add(gameplay->restart.spot, (v2){gameplay->restart.spot.x < game_state->vars.pitch.midfield_spot.x ? 65 : -65, own == PSI_NORTH ? 60 : -60});
			v2 direction = Vector2Normalize(Vector2Subtract(target, gameplay->restart.spot));
			if (gameplay->restart.manual) direction = gameplay->restart.aim;
			const f32 power = gameplay->restart.manual ? gameplay->restart.power : 0.7f;
			const b32 shot = state_machine->current_state == FS_PENALTY_KICK ||
				(gameplay->restart.manual ? gameplay->restart.action == GAME_CONTROL_SHOOT && state_machine->current_state != FS_THROW_IN :
				(state_machine->current_state == FS_FREE_KICK && Vector2Distance(target, gameplay->restart.spot) < 280));
			entity_ball_set_last_touch(ball, gameplay->restart.taker, game_state);
			if (shot) entity_ball_shoot(ball, gameplay->restart.taker, direction, 0.15f, power, false, game_state);
			else if (state_machine->current_state != FS_THROW_IN && (gameplay->restart.manual ? gameplay->restart.action == GAME_CONTROL_LONG_PASS : state_machine->current_state == FS_CORNER_KICK || state_machine->current_state == FS_GOAL_KICK))
				entity_ball_long_pass(ball, gameplay->restart.taker, direction, gameplay->restart.manual ? power : state_machine->current_state == FS_CORNER_KICK ? 0.5f : 0.75f, false, game_state);
			else entity_ball_short_pass(ball, gameplay->restart.taker, V2_TO_V3(direction, 0), gameplay->restart.manual ? power : 0.35f, game_state);
			if (gameplay->restart.manual) input_reset(&game_state->input_manager->game_controllers[gameplay->restart.controller_id]);
			GLOGF_INFO(LS_GAME, "RESTART delivered manual=%d power=%.2f aim=%.2f,%.2f", gameplay->restart.manual, power, direction.x, direction.y);
			gameplay->restart.prepared = false;
			gameplay_fire(FT_PLAY_RESUMED, game_state);
		}
		break;
	case FS_NONE:
	case FS_HALF_TIME:
	case FS_EXTRA_TIME:
	case FS_PENALTIES:
	case FS_PLAY:
	case FS_COUNT:
		break;
	}
}

void gameplay_award_foul(struct entity *offender, struct entity *victim, struct game_state *game_state)
{
	if (game_state->gameplay.state_machine.current_state != FS_PLAY ||
		offender->player.team_side == victim->player.team_side) return;
	struct gameplay *gameplay = &game_state->gameplay;
	const enum pitch_side defending = game_state->vars.teams[offender->player.team_side].play_context.pitch_side;
	const b32 penalty = CheckCollisionPointRec(victim->transform.position, game_state->vars.pitch.goal[defending].penalty_box);
	gameplay->restart.team = victim->player.team_side;
	gameplay->restart.spot = penalty ? game_state->vars.pitch.goal[defending].penalty_spot : victim->transform.position;
	gameplay_fire(penalty ? FT_PENALTY_AWARDED : FT_FOUL, game_state);
}

void gameplay_prepare_restart(struct game_state *game_state)
{
	struct gameplay *gameplay = &game_state->gameplay;
	struct entity *ball = game_state->vars.ball.entity;
	const enum state_machine_football_state state = gameplay->state_machine.current_state;
	const enum team_side last = entity_ball_team_in_possession(ball, game_state);
	if (state == FS_THROW_IN || state == FS_CORNER_KICK || state == FS_KICKOFF)
		gameplay->restart.team = last == TS_HOME ? TS_AWAY : TS_HOME;
	if (state == FS_KICKOFF && ball->ball.last_touch_entity_id == EID_NOT_VALID) gameplay->restart.team = TS_HOME;
	if (state == FS_GOAL_KICK) {
		const enum pitch_side side = ball->transform.position.y < game_state->vars.pitch.midfield_spot.y ? PSI_NORTH : PSI_SOUTH;
		gameplay->restart.team = game_state->vars.teams[TS_HOME].play_context.pitch_side == side ? TS_HOME : TS_AWAY;
	}
	gameplay->restart.spot = ball->transform.position;
	gameplay->restart.start_time = game_state->time;
	gameplay->restart.taker = EID_NOT_VALID;
	gameplay->restart.manual = false;
	gameplay->restart.charging = false;
	gameplay->restart.power = 0;
	gameplay->restart.action = -1;
	for (usize i = 0; i < MAX_HUMAN_PLAYERS; ++i) {
		struct human_player *human = &game_state->human_players[i];
		if (human->enabled && human->team_side == gameplay->restart.team && game_state->input_manager) {
			gameplay->restart.manual = true;
			gameplay->restart.controller_id = human->game_controller_id;
		}
	}
	f32 nearest = FLT_MAX;
	{
	FOR_EACH_ENTITY(player, game_state->entity_repository) {
		if (!player->header.alive || player->header.type != ET_PLAYER || player->player.team_side != gameplay->restart.team) continue;
		if (entity_player_is_goalie(player) != (state == FS_GOAL_KICK)) continue;
		f32 distance = Vector2Distance(player->transform.position, gameplay->restart.spot);
		if (distance < nearest) { nearest = distance; gameplay->restart.taker = player->header.id; }
	}
	}
	if (gameplay->restart.taker == EID_NOT_VALID) return;
	const enum pitch_side own = game_state->vars.teams[gameplay->restart.team].play_context.pitch_side;
	const f32 forward = own == PSI_NORTH ? 1 : -1;
	const v2 goal = game_state->vars.pitch.goal[own == PSI_NORTH ? PSI_SOUTH : PSI_NORTH].center;
	gameplay->restart.aim = Vector2Normalize(Vector2Subtract(goal, gameplay->restart.spot));
	if (state == FS_KICKOFF || state == FS_THROW_IN)
		gameplay->restart.aim = Vector2Normalize((v2){gameplay->restart.spot.x < game_state->vars.pitch.midfield_spot.x ? 65 : -65, forward * 60});
	for (usize i = 0; i < MAX_HUMAN_PLAYERS; ++i)
		if (game_state->human_players[i].enabled && game_state->human_players[i].team_side == gameplay->restart.team)
			game_state->human_players[i].controlled_player.entity_id = gameplay->restart.taker;
	u32 wall = 0;
	FOR_EACH_ENTITY(player, game_state->entity_repository) {
		if (!player->header.alive || player->header.type != ET_PLAYER) continue;
		v2 pos = football_tactics_get_target_position(player, ball, game_state);
		const b32 attacking = player->player.team_side == gameplay->restart.team;
		if (state == FS_KICKOFF && !entity_player_is_goalie(player)) {
			const f32 middle = game_state->vars.pitch.midfield_spot.y;
			const f32 direction = game_state->vars.teams[player->player.team_side].play_context.pitch_side == PSI_NORTH ? 1 : -1;
			if ((pos.y - middle) * direction > -35) pos.y = middle - direction * 45;
		}
		if (state == FS_CORNER_KICK && !entity_player_is_goalie(player)) {
			const u32 idx = player->player.team_formation_position_idx;
			if (idx >= 5) pos = (v2){goal.x + ((int)(idx % 3) - 1) * 45 + (attacking ? -8 : 8), goal.y - forward * (65 + (idx % 2) * 45)};
		}
		if (state == FS_FREE_KICK && !attacking && !entity_player_is_goalie(player) && wall < 3) {
			v2 toward_goal = Vector2Normalize(Vector2Subtract(goal, gameplay->restart.spot));
			pos = Vector2Add(gameplay->restart.spot, Vector2Scale(toward_goal, 90));
			pos.x += ((int)wall++ - 1) * 20;
		}
		if (state == FS_PENALTY_KICK && !entity_player_is_goalie(player)) {
			pos.y = gameplay->restart.spot.y - forward * 115;
		}
		if (player->header.id == gameplay->restart.taker) pos = Vector2Add(gameplay->restart.spot, (v2){0, -forward * 14});
		else if (!entity_player_is_goalie(player)) {
			const f32 minimum = state == FS_KICKOFF || !attacking ? 95 : 35;
			v2 away = Vector2Subtract(pos, gameplay->restart.spot);
			if (Vector2Length(away) < minimum) {
				if (Vector2Length(away) < 1) away = (v2){1, 0};
				pos = Vector2Add(gameplay->restart.spot, Vector2Scale(Vector2Normalize(away), minimum));
			}
		}
		pos.x = Clamp(pos.x, game_state->vars.pitch.field.x + 8, game_state->vars.pitch.field.x + game_state->vars.pitch.field.width - 8);
		pos.y = Clamp(pos.y, game_state->vars.pitch.field.y + 8, game_state->vars.pitch.field.y + game_state->vars.pitch.field.height - 8);
		player->transform.position = pos;
		player->physics.velocity = Vector2Zero();
		RESET_COMPONENT(&player->player_controls);
	}
	gameplay->restart.prepared = true;
	GLOGF_INFO(LS_GAME, "RESTART %s team=%s", state_machine_football_state_string[state], team_side_to_string[gameplay->restart.team]);
}

// Use the same controller abstraction for keyboard and gamepad. Releasing the
// selected button delivers the kick; reaching full power never fires it early.
b32 gameplay_update_restart_input(struct game_state *game_state)
{
	struct gameplay *gameplay = &game_state->gameplay;
	struct game_controller *controller = &game_state->input_manager->game_controllers[gameplay->restart.controller_id];
	v2 aim = input_movement_vector(controller);
	const rect_t field = game_state->vars.pitch.field;
	const v2 spot = gameplay->restart.spot;
	// Do not allow a restart to be aimed immediately outside the field.
	if (spot.x <= field.x + 2) aim.x = fmaxf(aim.x, 0);
	if (spot.x >= field.x + field.width - 2) aim.x = fminf(aim.x, 0);
	if (spot.y <= field.y + 2) aim.y = fmaxf(aim.y, 0);
	if (spot.y >= field.y + field.height - 2) aim.y = fminf(aim.y, 0);
	if (Vector2LengthSqr(aim) > 0.01f) gameplay->restart.aim = Vector2Normalize(aim);
	const enum game_control actions[] = {GAME_CONTROL_SHOOT, GAME_CONTROL_AUTOMATIC_PASS, GAME_CONTROL_PASS, GAME_CONTROL_LONG_PASS};
	if (!gameplay->restart.charging) {
		for (usize i = 0; i < ARRAY_LEN(actions); ++i) {
			if (controller->state.current[actions[i]].digital.pressed) {
				gameplay->restart.action = actions[i];
				gameplay->restart.charge_start = game_state->time;
				gameplay->restart.charging = true;
				break;
			}
		}
	}
	if (!gameplay->restart.charging) return false;
	gameplay->restart.power = Clamp(0.15f + (game_state->time - gameplay->restart.charge_start) * (0.85f / 1.2f), 0.15f, 1.0f);
	return controller->state.current[gameplay->restart.action].digital.released;
}
