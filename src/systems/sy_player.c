#include "components.h"
#include "entities.h"
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

void systems_player(struct game_state *game_state, struct input *input)
{
    UNUSED(input);

    struct entity *ball_entity =
	entities_entity_get(game_state->entity_repository, game_state->vars.ball_entity_id);
    DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);
    UNUSED(ball_physics_3d);

    game_state->vars.player_on_the_ball_entity_id = EID_NOT_VALID;
    f32 min_distance_from_the_ball = UINT32_MAX;

    struct entity *last_entity_that_touched_ball =
	entities_entity_get(game_state->entity_repository, ball->last_touch_entity_id);

    enum team_side team_idx_last_ball_possession =
	last_entity_that_touched_ball ? last_entity_that_touched_ball->player.team_side : TS_COUNT;

    FOR_EACH_ENTITY(entity, game_state->entity_repository)
    {
	if (!entities_is_entity_alive_and_has_components(
		entity, 5, C_TRANSFORM, C_PHYSICS, C_SPRITE, C_PLAYER, C_COLLIDER)) {
	    continue;
	}

	DECLARE_ENTITY_COMPONENT(entity, transform);
	DECLARE_ENTITY_COMPONENT(entity, physics);
	DECLARE_ENTITY_COMPONENT(entity, player);
	DECLARE_ENTITY_COMPONENT(entity, collider);

	struct state_machine_player *state_machine = player->state_machine;

	struct component_player_controls *player_controls = &entity->player_controls;
	RESET_COMPONENT(player_controls);

	collider->colliders[PCT_PLAYER].props.circle.radius =
	    game_state->constants.player_control_radius;

	switch (state_machine->current_state) {
	case PS_NONE:
	    break;
	case PS_NORMAL: {
	    if (game_state->human_players[0].controlled_player_entity_id == entity_id) {
		// Player is human controlled
		player_controls->steering = game_state->human_players[0].controls.steering;

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
	    } else {
		// Player is AI controlled
		if (game_state->constants.ai_enabled) {

		    v2 target_pos;
		    enum player_ai_kick_decision kick_decision;
		    enum player_ai_state ai_state =
			ai_player_think(entity, &target_pos, &kick_decision, game_state);

		    switch (ai_state) {
		    case PAS_KEEP_POSITION:
			player_controls->steering = Vector2Zero();
			break;
		    case PAS_SEEK_POSITION: {
			player_controls->steering = steering_seek(transform->position,
								  physics->velocity,
								  physics->max_velocity,
								  target_pos);
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
		    }
		}
	    }

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
	    // Player without the ball is 10% faster than the same player with the ball
	    physics->max_velocity =
		player->current_max_velocity *
		(game_state->vars.player_on_the_ball_entity_id == entity_id ? 1 : 1.1f);

	    entities_player_apply_force(entity_id, force_to_apply, game_state);
	    entities_player_apply_velocity(entity_id, game_state);

	    // Velocity has been applied to player after this point

	    b32 is_colliding_with_ball =
		CheckCollisionCircles(transform->position,
				      collider->colliders[0].props.circle.radius,
				      ball_transform->position,
				      ball_collider->colliders[0].props.circle.radius) &&
		// Player height
		ball_transform->virtual_height < game_state->constants.player_control_height;

	    if (is_colliding_with_ball && (game_state->time - player->last_kick_time) > 0.1f) {
		f32 distance_from_the_ball =
		    Vector2DistanceSqr(transform->position, ball_transform->position);
		if (distance_from_the_ball < min_distance_from_the_ball) {
		    game_state->vars.player_on_the_ball_entity_id = entity_id;
		    min_distance_from_the_ball = distance_from_the_ball;

		    ball->pass_target_entity_id = EID_NOT_VALID;
		}
	    } else {
		player->control_ball = true;
		player->time_spent_in_precision_control = 0;
	    }

	    if (!is_colliding_with_ball && team_idx_last_ball_possession != player->team_side &&
		input->devices[0].buttons[IDBT_BUTTON_LEFT].pressed) {
		entities_player_state_machine_change_state_to(
		    player->state_machine, PS_SLIDING_TACKLE, game_state);
	    }

	    // Calculate frame to display
	    entities_player_calculate_running_frame(entity_id, physics->velocity, 3, game_state);

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
		entities_player_state_machine_change_state_to(
		    player->state_machine, PS_NORMAL, game_state);
		break;
	    }

	    // Handle max velocity
	    physics->max_velocity = game_state->constants.player_run_max_velocity;

	    entities_player_apply_force(entity_id, force_to_apply, game_state);
	    entities_player_apply_velocity(entity_id, game_state);

	    break;
	}
	case PS_FALL: {
	    struct state_machine_player_fall_data *fall_state_data =
		&player->state_machine->state_data.fall;

	    if (!fall_state_data->animation_completed) {
		struct assets_animation *animation =
		    assets_animation_get_by_id(game_state->assets.animation_cache,
					       player->fall_animations[player->last_direction]);
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

	    entities_player_apply_force(entity_id, force_to_apply, game_state);
	    entities_player_apply_velocity(entity_id, game_state);

	    if (fall_state_data->animation_completed &&
		HAS_TIME_PASSED(game_state->time, fall_state_data->animation_end_time, 0.5f)) {
		entities_player_state_machine_change_state_to(
		    player->state_machine, PS_NORMAL, game_state);
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
