#include "components.h"
#include "entities.h"
#include "e_ball.h"
#include "e_player.h"
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

	struct entity *ball_entity = game_state->vars.ball_entity;

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

		if (!entity_is_alive_and_has_components(
				entity, 5, C_TRANSFORM, C_PHYSICS, C_SPRITE, C_PLAYER, C_COLLIDER)) {
			continue;
		}

		DECLARE_ENTITY_COMPONENT(entity, player);
		DECLARE_ENTITY_COMPONENT(entity, collider);

		struct state_machine_player *state_machine = &player->state_machine;

		struct component_player_controls *player_controls = &entity->player_controls;
		RESET_COMPONENT(player_controls);

		struct entity *last_entity_that_touched_ball =
			entity_get(game_state->entity_repository, ball->last_touch_entity_id);

		enum team_side team_idx_last_ball_possession =
			last_entity_that_touched_ball ? last_entity_that_touched_ball->player.team_side :
											TS_COUNT;

		collider->colliders[PCT_PLAYER].props.circle.radius =
			game_state->constants.player_control_radius;

		// Player is human controlled
		struct game_controller *game_controller =
			&game_state->input_manager
				 ->game_controllers[game_state->human_players[0].game_controller_id];
		struct game_controller_state *game_controller_state = &game_controller->current_state;

		player_controls->steering = game_controller_state->movement;

		const u8 game_controller_idx = 0;

		switch (state_machine->current_state) {
		case PS_NONE:
			break;
		case PS_NORMAL: {

			if (!entity->player.is_colliding_with_ball && game_controller_state->shoot) {
				if (ball_transform->virtual_height > 20) {
					entity_player_state_machine_change_state_to(
						&player->state_machine, PS_HEADER, game_state);
				}
			} else if (!entity->player.is_colliding_with_ball && game_controller_state->tackle) {
				if (team_idx_last_ball_possession != player->team_side) {
					entity_player_state_machine_change_state_to(
						&player->state_machine, PS_SLIDING_TACKLE, game_state);
				}
			} else {
				if (game_controller_state->charge.ready) {
					switch (game_controller_state->charge.type) {
					case ACT_SHOOT: {
						player_controls->shot = game_controller_state->charge.power;
						break;
					}
					case ACT_PASS: {
						player_controls->pass = game_controller_state->charge.power;
						if ((game_controller_state->charge.ended -
							 game_controller_state->charge.started) < 0.15f) {
							player_controls->automatic_pass = true;
						}
						break;
					}
					case ACT_LONG_PASS: {
						player_controls->long_pass = game_controller_state->charge.power;
						break;
					}
					}
				}
			}

			break;
		}
		case PS_SLIDING_TACKLE:
		case PS_FALL:
		case PS_HEADER:
		case PS_GOALIE:
		case PS_THROW_IN:
		case PS_COUNT:
			break;
		}
	}
}
