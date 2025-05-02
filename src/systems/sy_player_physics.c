#include "components.h"
#include "e_ball.h"
#include "entities.h"
#include "football.h"
#include "game.h"
#include "input.h"
#include "macro.h"
#include <math_ext.h>
#include <raymath.h>
#include "ai.h"
#include "types.h"

void systems_player_physics(struct game_state *game_state)
{
	struct entity *ball_entity = game_state->vars.ball_entity;

	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
	UNUSED(ball_physics_3d);
	UNUSED(ball);
	UNUSED(ball_collider);

	entity_id_t player_in_possession_entity_id = ball_player_in_possession(ball_entity, game_state);

	FOR_EACH_ENTITY(entity, game_state->entity_repository)
	{
		if (!entity_is_alive_and_has_components(
				entity, 5, C_TRANSFORM, C_PHYSICS, C_SPRITE, C_PLAYER, C_COLLIDER)) {
			continue;
		}

		DECLARE_ENTITY_COMPONENT(entity, transform);
		DECLARE_ENTITY_COMPONENT(entity, physics);
		DECLARE_ENTITY_COMPONENT(entity, player);
		DECLARE_ENTITY_COMPONENT(entity, collider);
		DECLARE_ENTITY_COMPONENT(entity, sprite);

		struct state_machine_player *state_machine = player->state_machine;

		struct component_player_controls *player_controls = &entity->player_controls;

		collider->colliders[PCT_PLAYER].props.circle.radius =
			game_state->constants.player_control_radius;

		switch (state_machine->current_state) {
		case PS_NONE:
			break;
		case PS_NORMAL: {
			v2 effective_steering = player_controls->steering;
			// If charging kick we seek the ball
			if (entity_id == game_state->human_players[0].controlled_player_entity_id &&
				game_state->human_players[0].controls.charge.charging) {
				effective_steering = Vector2Normalize(
					Vector2Subtract(ball_transform->position, transform->position));
			}
			v2 force_to_apply = Vector2Zero();
			v2 normalized_velocity = Vector2Normalize(physics->velocity);
			f32 steering_magnitude = Vector2Length(effective_steering);
			f32 current_velocity = Vector2Length(physics->velocity);

			// Run force
			force_to_apply =
				Vector2Scale(effective_steering, game_state->constants.player_movement_force);

			// Apply drag if no steering applied
			if (steering_magnitude < 0.01f) {
				if (current_velocity < 3) {
					physics->velocity = Vector2Zero();
				} else {
					Vector2 drag = Vector2Scale(Vector2Negate(normalized_velocity),
												game_state->constants.player_movement_drag);
					force_to_apply = Vector2Add(force_to_apply, drag);
				}
			}

			// Handle max velocity
			f32 player_run_max_velocity = game_state->constants.player_run_max_velocity;
			f32 initial_max_velocity = player_run_max_velocity * 0.5f;
			physics->max_velocity = initial_max_velocity;

			f32 projection = Vector2ProjectScalar(physics->velocity, effective_steering);
			if (projection > 0 && player->current_max_velocity < player_run_max_velocity) {
				player->current_max_velocity += 50 * game_state->dt;
				player->current_max_velocity =
					fminf(player->current_max_velocity, player_run_max_velocity);
			} else if (current_velocity < player->current_max_velocity) {
				player->current_max_velocity -= 100 * game_state->dt;
				player->current_max_velocity =
					fmaxf(initial_max_velocity, player->current_max_velocity);
			}
			// Player without the ball is 15% faster than the same player with the ball
			physics->max_velocity = player->current_max_velocity *
									(player_in_possession_entity_id == entity_id ? 1 : 1.15f);

			player_physics_apply_force(entity, force_to_apply, game_state);
			player_physics_apply_velocity(entity, game_state);

			// Calculate frame to display
			player_anim_calculate_running_frame(entity,
												game_state->gfx.player.stand_frames,
												game_state->gfx.player.run_animations,
												physics->velocity,
												3,
												game_state);

			break;
		}
		case PS_GOALIE:
			break;
		case PS_THROW_IN:
		case PS_HEADER:
		case PS_SLIDING_TACKLE: {
			struct state_machine_player_tackle_data *tackle_state_data =
				&player->state_machine->state_data.tackle;

			v2 force_to_apply = Vector2Zero();
			if (Vector2Length(physics->velocity) > 3) {
				// Apply friction
				force_to_apply = Vector2Scale(Vector2Negate(Vector2Normalize(physics->velocity)),
											  7500); // TODO: this should depend on type of pitch
			} else if (HAS_TIME_PASSED(game_state->time, tackle_state_data->start_time, 0.5f)) {
				player_state_machine_change_state_to(player->state_machine, PS_NORMAL, game_state);
				break;
			}

			// Handle max velocity
			physics->max_velocity = game_state->constants.player_run_max_velocity;

			player_physics_apply_force(entity, force_to_apply, game_state);
			player_physics_apply_velocity(entity, game_state);

			break;
		}
		case PS_FALL: {
			struct state_machine_player_fall_data *fall_state_data =
				&player->state_machine->state_data.fall;

			if (!fall_state_data->animation_completed) {
				struct assets_animation *animation = assets_animation_get_by_id(
					game_state->assets.animation_cache,
					game_state->gfx.player.fall_animations[sprite->orientation]);
				f32 animation_completion =
					Clamp((game_state->time - fall_state_data->animation_start_time) /
							  (animation->delay * animation->frames_count),
						  0,
						  1);
				const u16 new_frame_id =
					floorf(animation_completion * (animation->frames_count - 1));
				if (animation_completion >= 1) {
					animation_completion = 1;
					fall_state_data->animation_end_time = game_state->time;
					fall_state_data->animation_completed = true;
				}
				entity->sprite.frame_id = animation->frames[new_frame_id];
			}

			v2 force_to_apply = Vector2Zero();
			if (fall_state_data->animation_completed) {
				// Apply friction
				force_to_apply = Vector2Scale(Vector2Negate(Vector2Normalize(physics->velocity)),
											  30000); // TODO: this should depend on type of pitch
			}

			// Handle max velocity
			physics->max_velocity = game_state->constants.player_run_max_velocity;

			player_physics_apply_force(entity, force_to_apply, game_state);
			player_physics_apply_velocity(entity, game_state);

			if (fall_state_data->animation_completed &&
				HAS_TIME_PASSED(game_state->time, fall_state_data->animation_end_time, 1.f)) {
				player_state_machine_change_state_to(player->state_machine, PS_NORMAL, game_state);
				break;
			}

			break;
		}
		case PS_COUNT:
			break;
		}
		if (state_machine->current_state != PS_GOALIE) {
			layers_debug_queue_draw_text(
				game_state->layer_debug,
				TextFormat("%-8s %02d pos: [%7.2f, %7.2f] vel: %5.2f state: %s",
						   "Player",
						   entity_id,
						   transform->position.x,
						   transform->position.y,
						   VELOCITY_TO_KMH(Vector2Length(physics->velocity)),
						   state_machine_player_state_string[state_machine->current_state]));
		}
	}
}
