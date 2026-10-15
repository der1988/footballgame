#include "entity.h"
#include "entity_ball.h"
#include "events_router.h"
#include "football.h"
#include "gameplay.h"
#include "math_ext.h"
#include "log.h"
#include "game.h"
#include <string.h>
#include "constants.h"

const char *pitch_side_to_string[PSI_COUNT] = {"north", "south"};
const char *team_side_to_string[TS_COUNT] = {"home", "away"};
const char *save_direction_to_string[SD_COUNT] = {"left", "right"};
const char *save_height_to_string[SH_COUNT] = {"high", "low"};
const char *team_formation_to_string[TF_COUNT] = {
	"4-3-3", "4-4-2", "3-5-2", "3-4-3", "5-3-2", "4-3-1-2", "4-2-3-1", "5-4-1", "3-2-3-2"};

const char *state_machine_football_state_string[FS_COUNT] = {"none",
															 "kickoff",
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

const char *state_machine_football_trigger_string[FT_COUNT] = {"none",
															   "goal_scored",
															   "ball_out_for_throw_in",
															   "ball_out_for_corner",
															   "ball_out_for_goal_kick",
															   "celebration_ended",
															   "play_resumed", "foul", "penalty_awarded"};

STATE_MACHINE_FORWARD_DECLARE_FUNCTIONS(football, struct gameplay);
STATE_MACHINE_IMPLEMENT_FUNCTIONS(football, struct gameplay);

// The byline the ball crossed, read off where it sits right now.
static enum pitch_side crossed_pitch_side(v2 ball_pos, struct pitch *pitch)
{
	return ball_pos.y < pitch->field.y + pitch->field.height * 0.5f ? PSI_NORTH : PSI_SOUTH;
}

static void place_ball(v2 spot, struct game_state *game_state)
{
	entity_ball_set_position_and_velocity(
		game_state->vars.ball.entity, V2_TO_V3(spot, BALL_RESTART_HEIGHT), Vector3Zero());
}

enum state_machine_football_trigger football_classify_ball_out(enum event_type event,
															   struct game_state *game_state)
{
	if (event == EVT_BALL_OOB_EAST || event == EVT_BALL_OOB_WEST) {
		return FT_BALL_OUT_FOR_THROW_IN;
	}

	const enum pitch_side side = event == EVT_BALL_OOB_NORTH ? PSI_NORTH : PSI_SOUTH;
	const enum team_side last_touch =
		entity_ball_team_in_possession(game_state->vars.ball.entity, game_state);

	// The team defending that byline put it out, so the attackers get a corner.
	return game_state->vars.teams[last_touch].play_context.pitch_side == side ?
			   FT_BALL_OUT_FOR_CORNER :
			   FT_BALL_OUT_FOR_GOAL_KICK;
}

static void state_machine_football_on_enter(struct gameplay *gameplay,
											enum state_machine_football_state from,
											enum state_machine_football_trigger trigger,
											enum state_machine_football_state to,
											struct game_state *game_state)
{
	UNUSED(from);
	UNUSED(trigger);

	struct state_machine_football *state_machine = &gameplay->state_machine;
	struct pitch *pitch = &game_state->vars.pitch;

	GLOGF_INFO_CTX(LS_STATE_MACHINE,
				   "football",
				   "entering state: %s",
				   state_machine_football_state_string[to]);

	// Reset state data
	memset(&state_machine->state_data, 0, sizeof(union state_machine_football_state_data));

	switch (to) {
	case FS_GOAL:
		state_machine->state_data.goal.start_time = game_state->time;
		break;
	case FS_KICKOFF:
		place_ball(pitch->midfield_spot, game_state);
		break;
	case FS_THROW_IN: {
		const v2 ball_pos = game_state->vars.ball.entity->transform.position;
		const b32 west = ball_pos.x < pitch->field.x + pitch->field.width * 0.5f;
		place_ball(
			(v2){.x = west ? pitch->field.x : pitch->field.x + pitch->field.width,
				 .y = Clamp(ball_pos.y, pitch->field.y, pitch->field.y + pitch->field.height)},
			game_state);
	} break;
	case FS_CORNER_KICK: {
		const v2 ball_pos = game_state->vars.ball.entity->transform.position;
		const b32 west = ball_pos.x < pitch->field.x + pitch->field.width * 0.5f;
		const enum pitch_side side = crossed_pitch_side(ball_pos, pitch);
		place_ball((v2){.x = west ? pitch->field.x : pitch->field.x + pitch->field.width,
						.y = side == PSI_NORTH ? pitch->field.y :
												 pitch->field.y + pitch->field.height},
				   game_state);
	} break;
	case FS_GOAL_KICK: {
		const enum pitch_side side =
			crossed_pitch_side(game_state->vars.ball.entity->transform.position, pitch);
		const struct goal *goal = &pitch->goal[side];
		place_ball((v2){.x = goal->center.x,
						.y = goal->center.y +
							 pitch->penalty_box_height * 0.25f * (side == PSI_NORTH ? 1 : -1)},
				   game_state);
	} break;
	case FS_PENALTY_KICK:
	case FS_FREE_KICK:
		place_ball(gameplay->restart.spot, game_state);
		break;
	case FS_NONE:
	case FS_HALF_TIME:
	case FS_EXTRA_TIME:
	case FS_PENALTIES:
	case FS_PLAY:
	case FS_COUNT:
		break;
	}
	if (to == FS_KICKOFF || to == FS_THROW_IN || to == FS_CORNER_KICK || to == FS_GOAL_KICK || to == FS_FREE_KICK || to == FS_PENALTY_KICK) gameplay_prepare_restart(game_state);
}

static void state_machine_football_on_exit(struct gameplay *gameplay,
										   enum state_machine_football_state from,
										   enum state_machine_football_trigger trigger,
										   enum state_machine_football_state to,
										   struct game_state *game_state)
{
	UNUSED(gameplay);
	UNUSED(trigger);
	UNUSED(to);
	UNUSED(game_state);

	GLOGF_INFO_CTX(LS_STATE_MACHINE,
				   "football",
				   "exiting state: %s",
				   state_machine_football_state_string[from]);
}
