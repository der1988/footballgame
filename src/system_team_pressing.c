#include "system.h"
#include "entity_components.h"
#include "entity_ball.h"
#include "entity_player.h"
#include "entity.h"
#include "football.h"
#include "game.h"
#include "input.h"
#include "log.h"
#include "math_ext.h"
#include <raymath.h>
#include "ai.h"
#include "types.h"

static b32 is_human_asking_to_press(enum team_side team_side, struct game_state *game_state);

void system_team_pressing(struct game_state *game_state)
{
	struct entity *ball_entity = game_state->vars.ball.entity;

	for (usize i = 0; i < TS_COUNT; ++i) {
		game_state->vars.teams[i].play_context.player_in_charge_of_pressing = EID_NOT_VALID;
	}

	enum team_side team_in_possession = entity_ball_team_in_possession(ball_entity, game_state);
	enum team_side pressing_team = 1 - team_in_possession;

	// A human decides when his team closes down: he holds the shoot button to send someone at the
	// ball, and nobody goes while he doesn't. A team left to the AI presses on its own.
	if (game_human_players_count_by_team(game_state, pressing_team) > 0 &&
		!is_human_asking_to_press(pressing_team, game_state)) {
		return;
	}

	entity_player_select_press_player(ball_entity, pressing_team, 250.f, game_state);
}

static b32 is_human_asking_to_press(enum team_side team_side, struct game_state *game_state)
{
	for (usize i = 0; i < MAX_HUMAN_PLAYERS; ++i) {
		struct human_player *human_player = &game_state->human_players[i];

		if (!human_player->enabled || human_player->team_side != team_side) {
			continue;
		}

		struct game_controller *game_controller =
			&game_state->input_manager->game_controllers[human_player->game_controller_id];
		if (game_controller->state.current[GAME_CONTROL_SHOOT].digital.down) {
			return true;
		}
	}
	return false;
}
