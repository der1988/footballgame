#include "entities.h"
#include "football.h"
#include "game.h"
#include "types.h"
#include "assert.h"
#include "ai.h"
#include <raymath.h>

void systems_player_selection(struct game_state *game_state, struct input *input)
{
	UNUSED(input);

	struct entity *ball_entity =
		entities_entity_get(game_state->entity_repository, game_state->vars.ball_entity_id);
	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
	UNUSED(ball_physics_3d);
	UNUSED(ball_collider);

	v2 epicenter = ball_transform->position;
	if (Vector2Distance(epicenter, ball->future_bounce.position) > 100) {
		// If the ball is in flight and it's gonna bounce far from current ball position,
		// let's use bounce position instead to select which player is human controlled
		epicenter = ball->future_bounce.position;
	}

	entity_id_t player_on_the_ball_entity_id = game_state->vars.player_on_the_ball_entity_id;
	struct entity *player_on_the_ball_entity =
		entities_entity_get(game_state->entity_repository, player_on_the_ball_entity_id);

	for (usize i = 0; i < MAX_HUMAN_PLAYERS; ++i) {
		struct human_player *human_player = &game_state->human_players[i];

		b32 player_on_the_ball_exists_and_its_in_our_team =
			player_on_the_ball_entity &&
			player_on_the_ball_entity->player.team_side == human_player->team_side;

		b32 player_on_the_ball_exists_and_its_not_in_our_team =
			player_on_the_ball_entity &&
			player_on_the_ball_entity->player.team_side != human_player->team_side;

		entity_id_t nearest_teammate_entity_id = player_on_the_ball_exists_and_its_in_our_team ?
													 player_on_the_ball_entity_id :
													 EID_NOT_VALID;
		entity_id_t player_to_avoid_entity_id = EID_NOT_VALID;

		if (!HAS_TIME_PASSED(game_state->time, ball->last_kick_time, 0.5f) &&
			ball->last_kicker_entity_id != EID_NOT_VALID && player_on_the_ball_entity) {
			struct entity *last_kicker_entity =
				entities_entity_get(game_state->entity_repository, ball->last_kicker_entity_id);

			if (last_kicker_entity->player.team_side ==
				player_on_the_ball_entity->player.team_side) {
				player_to_avoid_entity_id = ball->last_kicker_entity_id;
			}
		}
		if ((player_on_the_ball_exists_and_its_in_our_team &&
			 player_on_the_ball_entity->player.kicked_ball) ||
			player_on_the_ball_exists_and_its_not_in_our_team ||
			player_on_the_ball_entity_id == EID_NOT_VALID) {
			nearest_teammate_entity_id = entities_player_find_nearest_teammate(
				game_state, player_to_avoid_entity_id, epicenter, human_player->team_side);
		}
		// TODO: this has to be fixed to support multiple human players
		// e.g.: we need to check whether the player is already controlled by a human
		if (nearest_teammate_entity_id != human_player->controlled_player_entity_id) {
			// human is controlling a new player compared to previous frame
			game_input_charge_reset(human_player);
			human_player->controlled_player_entity_id = nearest_teammate_entity_id;
		}
	}
}
