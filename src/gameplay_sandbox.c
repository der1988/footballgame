#include "football.h"
#include "game.h"
#include "gameplay.h"
#include "gameplay_sandbox.h"
#include "constants.h"

static const enum state_machine_football_state sandbox_transitions[FS_COUNT][FT_COUNT] = {
	[FS_PLAY] = {[FT_GOAL_SCORED] = FS_GOAL,
                 [FT_FOUL] = FS_FREE_KICK,
                 [FT_PENALTY_AWARDED] = FS_PENALTY_KICK,
				 [FT_BALL_OUT_FOR_THROW_IN] = FS_THROW_IN,
				 [FT_BALL_OUT_FOR_CORNER] = FS_CORNER_KICK,
				 [FT_BALL_OUT_FOR_GOAL_KICK] = FS_GOAL_KICK},
	[FS_GOAL] = {[FT_CELEBRATION_ENDED] = FS_KICKOFF},
	[FS_KICKOFF] = {[FT_PLAY_RESUMED] = FS_PLAY},
	[FS_THROW_IN] = {[FT_PLAY_RESUMED] = FS_PLAY},
	[FS_CORNER_KICK] = {[FT_PLAY_RESUMED] = FS_PLAY},
	[FS_FREE_KICK] = {[FT_PLAY_RESUMED] = FS_PLAY},
	[FS_PENALTY_KICK] = {[FT_PLAY_RESUMED] = FS_PLAY},
	[FS_GOAL_KICK] = {[FT_PLAY_RESUMED] = FS_PLAY},
};

static void sandbox_setup(struct game_state *game_state)
{
	gameplay_setup_common(SANDBOX_OUTFIELD_PLAYERS_PER_TEAM, game_state);

	struct gameplay *gameplay = &game_state->gameplay;
	ASSERT(gameplay->mode == GPM_SANDBOX);
	gameplay->data.sandbox.restart_delay = SANDBOX_RESTART_DELAY;
	gameplay->config.goal_celebration_time = gameplay->data.sandbox.restart_delay;
}

const struct gameplay_vtable gameplay_sandbox_vtable = {
	.name = "sandbox",
	.setup = sandbox_setup,
	.transitions = sandbox_transitions,
};
