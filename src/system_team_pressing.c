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

void system_team_pressing(struct game_state *game_state)
{
	struct entity *ball_entity = game_state->vars.ball.entity;

	for (usize i = 0; i < TS_COUNT; ++i) {
		game_state->vars.teams[i].play_context.player_in_charge_of_pressing = EID_NOT_VALID;
	}

	enum team_side team_in_possession = entity_ball_team_in_possession(ball_entity, game_state);
	entity_player_select_press_player(ball_entity, 1 - team_in_possession, 250.f, game_state);
}
