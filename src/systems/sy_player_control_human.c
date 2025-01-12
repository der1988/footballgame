#include "components.h"
#include "entities.h"
#include "football.h"
#include "game.h"
#include "input.h"
#include "macro.h"
#include <math_ext.h>
#include <raymath.h>
#include <string.h>
#include "ai.h"

void systems_player_control_human(struct game_state *game_state, struct input *input)
{
	UNUSED(memset);
	UNUSED(input);

	struct entity *ball_entity =
		entities_entity_get(game_state->entity_repository, game_state->vars.ball_entity_id);
	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
	UNUSED(ball_physics_3d);
	UNUSED(ball_collider);
	UNUSED(ball_transform);
	UNUSED(ball);

	FOR_EACH_ENTITY(entity, game_state->entity_repository)
	{
		if (game_state->human_players[0].controlled_player_entity_id != entity_id) {
			continue;
		}

		if (!entities_is_entity_alive_and_has_components(
				entity, 5, C_TRANSFORM, C_PHYSICS, C_SPRITE, C_PLAYER, C_COLLIDER)) {
			continue;
		}

		DECLARE_ENTITY_COMPONENT(entity, player);
		DECLARE_ENTITY_COMPONENT(entity, collider);

		struct state_machine_player *state_machine = player->state_machine;

		struct component_player_controls *player_controls = &entity->player_controls;
		RESET_COMPONENT(player_controls);

		struct entity *last_entity_that_touched_ball =
			entities_entity_get(game_state->entity_repository, ball->last_touch_entity_id);

		enum team_side team_idx_last_ball_possession =
			last_entity_that_touched_ball ? last_entity_that_touched_ball->player.team_side :
											TS_COUNT;

		collider->colliders[PCT_PLAYER].props.circle.radius =
			game_state->constants.player_control_radius;

		switch (state_machine->current_state) {
		case PS_NONE:
			break;
		case PS_NORMAL: {

			// Player is human controlled
			player_controls->steering = game_state->human_players[0].controls.steering;

			if (!entity->player.is_colliding_with_ball &&
				// team_idx_last_ball_possession != player->team_side &&
				// TODO: (marco) is this what we want?
				input->devices[0].buttons[IDBT_BUTTON_LEFT].pressed) {
				entities_player_state_machine_change_state_to(
					player->state_machine, PS_SLIDING_TACKLE, game_state);
			} else {
				if (game_state->human_players[0].controls.charge.ready) {
					switch (game_state->human_players[0].controls.charge.type) {
					case ACT_SHOOT: {
						player_controls->shot = game_state->human_players[0].controls.charge.power;
						break;
					}
					case ACT_PASS: {
						player_controls->pass = game_state->human_players[0].controls.charge.power;
						if ((game_state->human_players[0].controls.charge.ended -
							 game_state->human_players[0].controls.charge.started) < 0.15f) {
							player_controls->automatic_pass = true;
						}
						break;
					}
					case ACT_LONG_PASS: {
						player_controls->long_pass =
							game_state->human_players[0].controls.charge.power;
						break;
					}
					}
				}
			}

			break;
		}
		case PS_GOALIE:
		case PS_THROW_IN:
		case PS_HEADER:
		case PS_SLIDING_TACKLE:
		case PS_FALL:
		case PS_COUNT:
			break;
		}
	}
}
