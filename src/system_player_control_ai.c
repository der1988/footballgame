#include "system.h"
#include "entity_components.h"
#include "entity.h"
#include "entity_ball.h"
#include "entity_player.h"
#include "football.h"
#include "game.h"
#include "input.h"
#include "macro.h"
#include "sprite.h"
#include "steering.h"
#include "math_ext.h"
#include <raymath.h>
#include <string.h>
#include "ai.h"
#include "types.h"

#define STAY_THERE_RADIUS 30.0f

void system_player_control_ai(struct game_state *game_state)
{
	UNUSED(memset);

	struct entity *ball_entity = game_state->vars.ball.entity;

	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
	UNUSED(ball_physics_3d);
	UNUSED(ball_collider);
	UNUSED(ball_transform);
	UNUSED(ball);

	FOR_EACH_ENTITY(entity, game_state->entity_repository)
	{
		struct human_player *human_player =
			game_input_human_player_by_player_entity_id(game_state, entity_id);
		if (human_player) {
			continue;
		}

		if (!entity_is_alive_and_has_components(
				entity, 5, C_TRANSFORM, C_PHYSICS, C_SPRITE, C_PLAYER, C_COLLIDER)) {
			continue;
		}

		DECLARE_ENTITY_COMPONENT(entity, transform);
		DECLARE_ENTITY_COMPONENT(entity, physics);
		DECLARE_ENTITY_COMPONENT(entity, player);
		DECLARE_ENTITY_COMPONENT(entity, collider);

		struct state_machine_player *state_machine = &player->state_machine;

		struct component_player_controls *player_controls = &entity->player_controls;
		RESET_COMPONENT(player_controls);

		collider->colliders[PCT_PLAYER].props.circle.radius = player->constants.control_radius;

		switch (state_machine->current_state) {
		case PS_NONE:
			break;
		case PS_NORMAL: {

			// Player is AI controlled
			if (game_state->constants.ai_enabled) {
				v2 target_pos = transform->position;
				f32 power = 0.0f;
				enum player_ai_action_decision action_decision = PAAD_NO_ACTION;
				enum player_ai_state ai_state =
					ai_player_think(entity, &target_pos, &action_decision, &power, game_state);

				switch (ai_state) {
				case PAS_KEEP_POSITION: {
					v2 target_position =
						football_tactics_get_target_position(entity, ball_entity, game_state);

					if (Vector2Distance(target_position, entity->transform.position) <
						STAY_THERE_RADIUS)
						player_controls->steering = Vector2Zero();
					else
						player_controls->steering = steering_arrive(transform->position,
																	physics->velocity,
																	physics->max_velocity,
																	target_position,
																	20);
					break;
				}
				case PAS_SEEK_POSITION: {
					// TODO: Using this instead of steering_seek has fixed the slow down when running
					// I'm not sure it's the right approach but it does the trick for now.
					player_controls->steering =
						Vector2Normalize(Vector2Subtract(target_pos, transform->position));
					break;
				}
				case PAS_ARRIVE_TO_POSITION: {
					player_controls->steering = steering_arrive(transform->position,
																physics->velocity,
																physics->max_velocity,
																target_pos,
																20);
					break;
				}
				case PAS_SET_STEERING: {
					player_controls->steering = target_pos;
					break;
				}
				}

				switch (action_decision) {
				case PAAD_SHOOT:
					player_controls->shot = power;
					break;
				case PAAD_PASS:
					player_controls->automatic_pass = true;
					player_controls->pass = power;
					break;
				case PAAD_LONG_PASS:
					player_controls->long_pass = power;
					break;
				case PAAD_TACKLE:
					player_controls->tackle = true;
					break;
				case PAAD_SLIDING_TACKLE:
					player_controls->sliding_tackle = true;
					break;
				case PAAD_HEADER_SHOOT:
					player_controls->header_shot = true;
					break;
				case PAAD_HEADER_PASS:
					player_controls->header_pass = true;
					break;
				case PAAD_NO_ACTION:
					break;
				}
			}

			break;
		}
		case PS_GOALIE:
		case PS_THROW_IN:
		case PS_HEADER: {
			// We want to pay the price of this logic only when colliding with the ball
			if (entity->player.is_colliding_with_ball) {
				// Keep steering toward the header target so the ball gets directed properly
				v2 goal_center = game_state->vars.pitch
									 .goal[game_state->vars.teams[1 - entity->player.team_side]
											   .play_context.pitch_side]
									 .center;
				f32 distance_from_goal = Vector2Distance(goal_center, entity->transform.position);

				if (distance_from_goal < 250) {
					player_controls->steering =
						Vector2Normalize(Vector2Subtract(goal_center, entity->transform.position));
				} else {
					struct entity *teammate;
					b32 found = entity_player_find_pass_target(
						entity,
						ORIENTATION_VECTORS[entity->sprite.orientation],
						180,
						0,
						game_state->constants.ball_pass_auto_max_distance,
						&teammate,
						game_state);
					if (found && teammate->player.state_machine.current_state != PS_GOALIE) {
						player_controls->steering = Vector2Normalize(Vector2Subtract(
							teammate->transform.position, entity->transform.position));
					}
				}
			}
			break;
		}
		case PS_TACKLE:
		case PS_SLIDING_TACKLE:
		case PS_FALL:
		case PS_COUNT:
			break;
		}
	}
}
