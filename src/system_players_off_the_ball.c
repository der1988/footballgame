#include "entity_components.h"
#include "entity.h"
#include "entity_ball.h"
#include "entity_player.h"
#include "football.h"
#include "game.h"
#include "input.h"
#include "log.h"
#include "macro.h"
#include "math_ext.h"
#include <raymath.h>
#include "ai.h"
#include "system.h"

void system_players_off_the_ball(struct game_state *game_state)
{

	struct entity *ball_entity = game_state->vars.ball.entity;

	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
	UNUSED(ball);
	UNUSED(ball_physics_3d);
	UNUSED(ball_collider);
	UNUSED(ball_transform);

	FOR_EACH_ENTITY(entity, game_state->entity_repository)
	{
		if (!entity_is_alive_and_has_components(
				entity, 5, C_TRANSFORM, C_PHYSICS, C_SPRITE, C_PLAYER, C_COLLIDER)) {
			continue;
		}

		DECLARE_ENTITY_COMPONENT(entity, player);

		struct state_machine_player *state_machine = &player->state_machine;

		struct component_player_controls *player_controls = &entity->player_controls;

		switch (state_machine->current_state) {
		case PS_NONE:
			break;
		case PS_NORMAL: {
			// A player mid-skid is committed to the turn and starts nothing new
			if (state_machine->state_data.normal.dribble.active) {
				break;
			}

			// Header
			if (player_controls->header_shot || player_controls->header_pass) {
				entity_player_header_start(entity,
										   player_controls->header_shot ? HEADER_TYPE_SHOOT :
																		  HEADER_TYPE_PASS,
										   game_state);
			}
			// Sliding tackle
			else if (player_controls->sliding_tackle) {
				state_machine_player_fire(&player->state_machine, PT_SLIDE_TACKLE, game_state);
			}
			// Tackle
			else if (player_controls->tackle) {
				state_machine_player_fire(&player->state_machine, PT_TACKLE, game_state);
			}
			break;
		}
		case PS_TACKLE: {
			if (player->is_colliding_with_ball) {
				f32 tackle_force =
					Vector2Length(Vector2Scale(entity->physics.velocity, player->collision_force)) /
					entity->physics.max_velocity;

				// Asking for nowhere in particular leaves the ball to come off the contact.
				v2 tackle_direction = state_machine->state_data.tackle.direction;
				if (IS_V2_ZERO(tackle_direction)) {
					tackle_direction = player->collision_direction;
				}

				// The pace goes off the ball first: what it was carrying was the run of the man
				// who had it, and standing the challenge up is what ends that run, so the ball
				// leaves on the strength of the challenge alone.
				entity_ball_tackle(ball_entity,
								   entity->header.id,
								   V2_TO_V3(tackle_direction, 0),
								   tackle_force,
								   game_state->constants.ball_tackle_force,
								   BTT_STANDING,
								   game_state);
				// Playing the ball takes it off him there and then. Nobody is in individual
				// possession of a ball that has just been knocked loose, but it is the
				// challenger's side that has it now.
				entity_player_gained_possession(entity, game_state);
				state_machine->state_data.tackle.touched_ball = true;
				state_machine->state_data.tackle.ball_touch_time = game_state->time;

				struct entity *victim_entity =
					entity_get(game_state->entity_repository,
							   state_machine->state_data.tackle.victim_entity_id);
				victim_entity->player.last_tackle_dispossession_time = game_state->time;
			}
			break;
		}
		case PS_SLIDING_TACKLE:
		case PS_FALL:
		case PS_TACKLED_RUN:
		case PS_HEADER:
		case PS_GOALIE:
		case PS_THROW_IN:
		case PS_COUNT:
			break;
		}
	}
}
