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

	entity_id_t player_in_possession_entity_id =
		entity_ball_player_in_possession(ball_entity, game_state);
	struct entity *player_in_possession_entity =
		entity_get(game_state->entity_repository, player_in_possession_entity_id);
	if (player_in_possession_entity) {
		enum team_side team_in_possession = player_in_possession_entity->player.team_side;
		entity_player_select_press_player(ball_entity, 1 - team_in_possession, 250.f, game_state);
	} else {
		for (usize i = 0; i < TS_COUNT; ++i) {
			entity_player_select_press_player(ball_entity, i, 250.f, game_state);
		}
	}
}
