#include "system.h"
#include "entity_components.h"
#include "entity.h"
#include "entity_ball.h"
#include "entity_player.h"
#include "football.h"
#include "game.h"
#include "input.h"
#include "macro.h"
#include "physics.h"
#include "sprite.h"
#include "math_ext.h"
#include <raymath.h>
#include <string.h>
#include "ai.h"
#include "types.h"
#include "constants.h"

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
		// A human-controlled keeper is still the AI's in goal, until he has the ball in his hands
		if (human_player && !entity_player_goalie_is_goalkeeping(entity)) {
			continue;
		}

		if (!entity_is_alive_and_has_components(
				entity, 5, C_TRANSFORM, C_PHYSICS, C_SPRITE, C_PLAYER, C_COLLIDER)) {
			continue;
		}

		DECLARE_ENTITY_COMPONENT(entity, physics);
		DECLARE_ENTITY_COMPONENT(entity, player);

		struct state_machine_player *state_machine = &player->state_machine;

		struct component_player_controls *player_controls = &entity->player_controls;
		RESET_COMPONENT(player_controls);
		player_controls->pass_target_entity_id = EID_NOT_VALID;

		switch (state_machine->current_state) {
		case PS_NONE:
			break;
		case PS_NORMAL: {

			// Player is AI controlled
			if (game_state->constants.ai_enabled) {
				const struct player_ai_decision decision = ai_player_think(entity, game_state);

				switch (decision.state) {
				case PAS_KEEP_POSITION: {
					v2 target_position =
						football_tactics_get_target_position(entity, ball_entity, game_state);

					// Standing on his spot he lets small shifts of it go, but while he is still
					// moving he keeps braking onto it
					if (Vector2Distance(target_position, entity->transform.position) <
							PLAYER_AI_STAY_THERE_RADIUS &&
						physics_is_at_rest(physics->velocity, PLAYER_AT_REST_SPEED))
						player_controls->steering = Vector2Zero();
					else
						player_controls->steering =
							entity_player_steer_arrive(entity, target_position, game_state);
					break;
				}
				case PAS_SEEK_POSITION: {
					player_controls->steering =
						entity_player_steer_seek(entity, decision.target_position);
					break;
				}
				case PAS_ARRIVE_TO_POSITION: {
					player_controls->steering =
						entity_player_steer_arrive(entity, decision.target_position, game_state);
					break;
				}
				case PAS_SET_STEERING: {
					player_controls->steering = decision.steering;
					break;
				}
				}

				switch (decision.action) {
				case PAAD_SHOOT:
					player_controls->shot = decision.power;
					break;
				case PAAD_PASS:
					player_controls->pass = decision.power;
					player_controls->pass_target_entity_id = decision.target_entity_id;
					break;
				case PAAD_LONG_PASS:
					player_controls->long_pass = decision.power;
					break;
				case PAAD_TACKLE:
					player_controls->tackle = true;
					player_controls->tackle_victim_entity_id = decision.target_entity_id;
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
		case PS_GOALIE: {
			const struct goalie_ai_decision decision = ai_goalie_think(entity, game_state);
			player_controls->steering = decision.steering;
			switch (decision.action) {
			case GAA_NO_ACTION:
				break;
			case GAA_THROW:
				player_controls->pass = decision.action_data.throw_ball.power;
				player_controls->pass_target_entity_id =
					decision.action_data.throw_ball.target_entity_id;
				break;
			case GAA_KICK:
				player_controls->long_pass = decision.action_data.kick.power;
				break;
			case GAA_PUT_DOWN:
				player_controls->put_down = true;
				break;
			case GAA_JUMP:
				player_controls->jump = true;
				break;
			case GAA_DIVE:
				player_controls->dive = true;
				player_controls->dive_data = decision.action_data.dive;
				break;
			case GAA_SMOTHER:
				player_controls->smother = true;
				player_controls->smother_direction = decision.action_data.smother.direction;
				break;
			}
			break;
		}
		case PS_THROW_IN: {
			// We want to pay the price of this logic only when colliding with the ball
			if (entity->player.is_colliding_with_ball) {
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
					b32 found =
						ai_pass_find_target(entity,
											ORIENTATION_VECTORS[entity->sprite.orientation],
											180,
											0,
											game_state->constants.ball_pass_auto_max_distance,
											false,
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
		case PS_HEADER: {
			// We want to pay the price of this logic only when colliding with the ball
			if (entity->player.is_colliding_with_ball) {
				if (state_machine->state_data.header.type == HEADER_TYPE_SHOOT) {
					v2 goal_center = game_state->vars.pitch
										 .goal[game_state->vars.teams[1 - entity->player.team_side]
												   .play_context.pitch_side]
										 .center;
					player_controls->steering =
						Vector2Normalize(Vector2Subtract(goal_center, entity->transform.position));
				} else {
					struct entity *teammate;
					if (ai_pass_find_target(entity,
											ORIENTATION_VECTORS[entity->sprite.orientation],
											180,
											0,
											game_state->constants.ball_pass_auto_max_distance,
											false,
											&teammate,
											game_state)) {
						player_controls->steering = Vector2Normalize(Vector2Subtract(
							teammate->transform.position, entity->transform.position));
						player_controls->pass_target_entity_id = teammate->header.id;
					}
				}
			}
			break;
		}
		case PS_TACKLE:
		case PS_SLIDING_TACKLE:
		case PS_FALL:
		case PS_TACKLED_RUN:
		case PS_COUNT:
			break;
		}
	}
}
