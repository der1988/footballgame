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

// Going through the man is the exception a challenge risks, not what it sets out to do.
// Rolled once per challenge, not per tick: the contact that carries the roll is reported for
// the single tick on which the ball is played.
#define TACKLE_KNOCKDOWN_PROBABILITY_PER_CHALLENGE 0.05f

void system_players_off_the_ball(struct game_state *game_state)
{

	struct entity *ball_entity = game_state->vars.ball.entity;

	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
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
		DECLARE_ENTITY_COMPONENT(entity, collider);

		struct state_machine_player *state_machine = &player->state_machine;

		struct component_player_controls *player_controls = &entity->player_controls;

		collider->colliders[PCT_PLAYER].props.circle.radius = player->constants.control_radius;

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
				struct entity *carrier_entity =
					entity_get(game_state->entity_repository, ball->last_touch_entity_id);
				const b32 carrier_is_opponent =
					carrier_entity && carrier_entity->header.id != entity->header.id &&
					carrier_entity->player.team_side != player->team_side;

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
								   true,
								   game_state);
				if (carrier_is_opponent) {
					carrier_entity->player.last_tackled_time = game_state->time;
				}
				// Playing the ball takes it off him there and then. Nobody is in individual
				// possession of a ball that has just been knocked loose, but it is the
				// challenger's side that has it now.
				entity_player_gained_possession(entity, game_state);
				state_machine->state_data.tackle.touched_ball = true;

				// Only the man carrying it can be put down, and only now and then. Who else
				// happens to be standing nearby has nothing to do with it, so nothing is
				// measured against them the way the sliding tackle measures its sensor.
				// Only a man on his feet can be put on the floor: firing a trigger the state he
				// is in does not answer panics, and PT_TACKLED is mapped from PS_NORMAL alone.
				if (carrier_is_opponent &&
					carrier_entity->player.state_machine.current_state == PS_NORMAL &&
					RANDOM_01(100000) < TACKLE_KNOCKDOWN_PROBABILITY_PER_CHALLENGE) {
					state_machine_player_fire(
						&carrier_entity->player.state_machine, PT_TACKLED, game_state);
					GLOGF_INFO_CTX(LS_GAME,
								   carrier_entity->header.name,
								   "player tackled by %s",
								   entity->header.name);
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
