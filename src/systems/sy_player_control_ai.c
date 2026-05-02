#include "components.h"
#include "entities.h"
#include "e_ball.h"
#include "e_player.h"
#include "football.h"
#include "game.h"
#include "input.h"
#include "macro.h"
#include "steering.h"
#include <math_ext.h>
#include <raymath.h>
#include <string.h>
#include "ai.h"
#include "types.h"

#define STAY_THERE_RADIUS 30.0f

static v2 get_target_position_based_on_tactics(struct entity *player_entity,
											   struct entity *ball_entity,
											   struct game_state *game_state);

void systems_player_control_ai(struct game_state *game_state)
{
	UNUSED(memset);

	struct entity *ball_entity = game_state->vars.ball_entity;

	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
	UNUSED(ball_physics_3d);
	UNUSED(ball_collider);
	UNUSED(ball_transform);
	UNUSED(ball);

	FOR_EACH_ENTITY(entity, game_state->entity_repository)
	{
		struct human_player *human_player =
			game_input_get_human_player_by_player_entity_id(game_state, entity_id);
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

		collider->colliders[PCT_PLAYER].props.circle.radius =
			game_state->constants.player_control_radius;

		switch (state_machine->current_state) {
		case PS_NONE:
			break;
		case PS_NORMAL: {

			// Player is AI controlled
			if (game_state->constants.ai_enabled) {
				v2 target_pos;
				f32 power;
				enum player_ai_action_decision action_decision;
				enum player_ai_state ai_state =
					ai_player_think(entity, &target_pos, &action_decision, &power, game_state);

				switch (ai_state) {
				case PAS_KEEP_POSITION: {
					v2 target_position =
						get_target_position_based_on_tactics(entity, ball_entity, game_state);

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
					struct entity *teammate =
						entity_player_find_pass_target(entity, 35, 300, game_state);
					if (teammate && teammate->player.state_machine.current_state != PS_GOALIE) {
						player_controls->steering = Vector2Normalize(Vector2Subtract(
							teammate->transform.position, entity->transform.position));
					}
				}
			}
			break;
		}
		case PS_SLIDING_TACKLE:
		case PS_FALL:
		case PS_COUNT:
			break;
		}
	}
}

static v2 get_target_position_based_on_tactics(struct entity *player_entity,
											   struct entity *ball_entity,
											   struct game_state *game_state)
{
	struct team_formation_position *team_formation_position =
		football_tactics_get_team_formation_position(
			TF_433, player_entity->player.team_formation_position_idx);

	v2 ball_normalized_pos = football_pitch_position_abs_to_normalized(
		game_state->vars.pitch.field, ball_entity->transform.position);

	v2 default_pos = team_formation_position->position[TM_DEFAULT];
	v2 target_pos;

	enum pitch_side player_pitch_side =
		game_state->vars.teams[player_entity->player.team_side].play_context.pitch_side;

	switch (game_state->vars.match.state) {
	case MS_PLAY: {
		if (ball_entity->ball.state_machine.current_state == BS_GK_HANDS) {
			target_pos = default_pos;
			break;
		}

		if (player_pitch_side == PSI_SOUTH) {
			if (ball_normalized_pos.y < 0.5f) {
				v2 attacking_pos = team_formation_position->position[TM_ATTACKING];
				target_pos = Vector2Lerp(attacking_pos, default_pos, ball_normalized_pos.y / 0.5f);
			} else {
				v2 defending_pos = team_formation_position->position[TM_DEFENDING];
				target_pos =
					Vector2Lerp(default_pos, defending_pos, (ball_normalized_pos.y - 0.5f) / 0.5f);
			}
		} else {
			if (ball_normalized_pos.y > 0.5f) {
				v2 attacking_pos = team_formation_position->position[TM_ATTACKING];
				target_pos =
					Vector2Lerp(default_pos, attacking_pos, (ball_normalized_pos.y - 0.5f) / 0.5f);
			} else {
				v2 defending_pos = team_formation_position->position[TM_DEFENDING];
				target_pos = Vector2Lerp(defending_pos, default_pos, ball_normalized_pos.y / 0.5f);
			}
		}

		// If ball is in the side lanes and player is on the other side of the pitch we need to
		// close the gap.
		if (ball_normalized_pos.x < 0.33f && target_pos.x > ball_normalized_pos.x) {
			f32 delta = target_pos.x - ball_normalized_pos.x;
			target_pos = Vector2Lerp(
				Vector2Add(target_pos, (v2){.x = -delta * (0.33f - ball_normalized_pos.x)}),
				target_pos,
				ball_normalized_pos.x / 0.33f);
		} else if (ball_normalized_pos.x > 0.66f && target_pos.x < ball_normalized_pos.x) {
			f32 delta = ball_normalized_pos.x - target_pos.x;
			target_pos = Vector2Lerp(target_pos,
									 Vector2Add(target_pos,
												(v2){.x = delta * (ball_normalized_pos.x - 0.66f)}),
									 (ball_normalized_pos.x - 0.66f) / 0.33f);
		}
	} break;
	case MS_KICKOFF:
	case MS_HALF_TIME:
	case MS_EXTRA_TIME:
	case MS_PENALTIES:
	case MS_GOAL:
	case MS_THROW_IN:
	case MS_GOAL_KICK:
	case MS_CORNER_KICK:
	case MS_PENALTY_KICK:
	case MS_FREE_KICK:
	case MS_COUNT:
		target_pos = default_pos;
		break;
	}

	if (player_pitch_side == PSI_NORTH) {
		target_pos.y = 1 - target_pos.y;
	}

	return football_pitch_position_normalized_to_abs(game_state->vars.pitch.field, target_pos);
}
