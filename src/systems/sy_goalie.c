#include "assets.h"
#include "components.h"
#include "entities.h"
#include "game.h"
#include "input.h"
#include "simulation.h"
#include "steering.h"
#include "types.h"
#include "macro.h"
#include "math_ext.h"
#include <raylib.h>
#include <raymath.h>
#include "constants.h"

static b32 protect_goal_predicate(v3 pos_to_check,
				  struct entity *seeker_entity,
				  struct game_state *game_state)
{
    f32 ball_distance_from_goal = Vector3Distance(
	pos_to_check, V2_TO_V3(game_state->vars.pitch.goal[seeker_entity->player.side].center, 0));
    return (football_pitch_is_position_inside_penalty_area(
		V3_TO_V2(pos_to_check), seeker_entity->player.side, game_state) &&
	    ball_distance_from_goal < 125 &&
	    pos_to_check.y < (game_state->constants.crossbar_height - 7));
}

void systems_goalie(struct game_state *game_state, struct input *input)
{
    UNUSED(input);

    struct entity *ball_entity =
	entities_entity_get(game_state->entity_repository, game_state->vars.ball_entity_id);
    declare_ball_entity_components(ball_entity);

    for_each_entity(entity, game_state->entity_repository) {
	if (!entities_is_entity_alive_and_has_components(
		entity, 6, C_TRANSFORM, C_PHYSICS, C_SPRITE, C_PLAYER, C_GOALIE, C_COLLIDER)) {
	    continue;
	}

	declare_entity_component(entity, player);
	if (player->state_machine->current_state != PS_GOALIE)
	    continue;
	declare_entity_component(entity, transform);
	declare_entity_component(entity, physics);
	declare_entity_component(entity, sprite);
	declare_entity_component(entity, goalie);
	declare_entity_component(entity, collider);

	struct state_machine_goalie *state_machine = goalie->state_machine;

	const v2 goal_center = game_state->vars.pitch.goal[player->side].center;

	const f32 ball_velocity = Vector3Length(ball_physics_3d->velocity);

	collider->colliders[PCT_PLAYER].props.circle.radius =
	    game_state->constants.player_control_radius;
	struct collider *goalie_collider = &collider->colliders[PCT_GOALIE];

	switch (state_machine->current_state) {
	case GS_NONE:
	    break;
	case GS_PROTECT_GOAL: {
	    if (entities_player_is_standing_goalie_colliding_with_ball(
		    entity,
		    V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
		    ball_collider->colliders[0].props.circle.radius,
		    game_state->constants.crossbar_height)) {
		// hold ball in hands
		entities_ball_held_by_goalie(
		    game_state->vars.ball_entity_id, entity_id, game_state);
		entities_goalie_state_machine_change_state_to(
		    goalie->state_machine, GS_HOLD_BALL, game_state);
		break;
	    }

	    v2 destination;
	    v2 direction;
	    v2 steering;
	    f32 force;

	    const f32 exit_max_speed = game_state->constants.player_run_max_velocity * 1.5f;

	    // Calculate whether goalie can reach the ball at its next bounce position
	    const b32 should_seek_ball_at_bounce_pos =
		entities_common_can_reach_position_in_time(transform->position,
							   ball->future_bounce_position,
							   game_state->dt *
							       ball->dts_to_future_bounce,
							   exit_max_speed) &&
		football_pitch_is_position_inside_penalty_area(
		    ball->future_bounce_position, player->side, game_state);

	    if (should_seek_ball_at_bounce_pos) {
		// seek ball future bounce position
		physics->max_velocity = exit_max_speed;
		force = game_state->constants.player_movement_force * 2;

		destination = ball->future_bounce_position;
		direction = Vector2Normalize(Vector2Subtract(destination, transform->position));
		steering = steering_arrive(
		    transform->position, physics->velocity, physics->max_velocity, destination, 10);
	    } else {
		// Calculate whether goalie can reach the ball using predicted positions
		v3 future_pos;
		time_t future_time;
		const b32 can_reach_ball =
		    entities_common_can_reach_future_ball_position(entity,
								   exit_max_speed,
								   ball->lookahead_positions,
								   protect_goal_predicate,
								   &future_pos,
								   &future_time,
								   game_state);

		if (can_reach_ball) {
		    physics->max_velocity = exit_max_speed;
		    force = game_state->constants.player_movement_force * 1;

		    destination = V3_TO_V2(future_pos);
		    direction = Vector2Normalize(Vector2Subtract(destination, transform->position));
		    steering = steering_seek(
			transform->position, physics->velocity, physics->max_velocity, destination);
		} else {
		    // ball isn't within reach, let's protect goal
		    physics->max_velocity = game_state->constants.player_run_max_velocity;
		    force = game_state->constants.player_movement_force;
		    direction =
			Vector2Normalize(Vector2Subtract(ball_transform->position, goal_center));
		    destination = Vector2Add(goal_center, Vector2Scale(direction, 20));
		    steering = steering_arrive(transform->position,
					       physics->velocity,
					       physics->max_velocity,
					       destination,
					       10);
		}
	    }

	    // Run force
	    const v2 force_to_apply = Vector2Scale(steering, force);

	    entities_player_apply_force(entity_id, force_to_apply, game_state);
	    entities_player_apply_velocity(entity_id, game_state);

	    entities_player_calculate_running_frame(entity_id, direction, 5, game_state);

	    if (ball_velocity > (10 * PIXELS_PER_METER) &&
		entities_player_should_dive(entity,
					    ball_entity,
					    &goalie->dive_direction,
					    &goalie->dive_height,
					    &goalie->dive_speed,
					    &goalie->dive_distance,
					    game_state)) {
		// Only dive if ball velocity is higher than 36km/h
		entities_goalie_state_machine_change_state_to(
		    goalie->state_machine, GS_DIVE, game_state);
	    }

	    break;
	}
	case GS_HOLD_BALL: {
	    if (!HAS_TIME_PASSED(game_state->time,
				 goalie->state_machine->state_data.hold_ball.hold_start_time,
				 1)) {
		break;
	    }
	    entities_ball_back_in_play(game_state->vars.ball_entity_id, game_state);
	    const v2 new_ball_position =
		Vector2Add(transform->position,
			   Vector2Scale((v2){.y = 1},
					goalie_collider->props.circle.offset.y +
					    goalie_collider->props.circle.radius +
					    ball_collider->colliders[0].props.circle.radius + 7));
	    entities_ball_set_position_and_velocity(game_state->vars.ball_entity_id,
						    V2_TO_V3(new_ball_position, 10),
						    Vector3Zero(),
						    game_state);
	    entities_ball_long_pass(
		game_state->vars.ball_entity_id, entity_id, (v2){.y = 1}, 0.7f, game_state);
	    entities_goalie_state_machine_change_state_to(
		goalie->state_machine, GS_PROTECT_GOAL, game_state);
	    break;
	}
	case GS_DIVE: {
	    enum save_direction direction = goalie->dive_direction;

	    struct state_machine_goalie_dive_data *dive_state_data =
		&goalie->state_machine->state_data.dive;

	    if (!dive_state_data->animation_first_phase_completed) {
		struct assets_animation *animation = assets_animation_get_by_id(
		    game_state->animation_cache,
		    goalie->dive_animations_start[SS_UP][direction][goalie->dive_height]);
		f32 animation_completion =
		    Clamp((game_state->time - dive_state_data->animation_start_time) /
			      (animation->delay * animation->frames_count),
			  0,
			  1);
		const u16 new_frame_id =
		    floorf(animation_completion * (animation->frames_count - 1));
		if (animation_completion >= 1) {
		    animation_completion = 1;
		    dive_state_data->animation_first_phase_completed = true;
		    dive_state_data->animation_first_phase_end_time = game_state->time;
		}
		sprite->frame_id = animation->frames[new_frame_id];
	    } else if ((dive_state_data->ball_held ||
			HAS_TIME_PASSED(game_state->time,
					dive_state_data->animation_first_phase_end_time,
					goalie->dive_speed * 0.35f)) &&
		       !dive_state_data->animation_completed) {
		if (!dive_state_data->animation_second_phase_started) {
		    dive_state_data->animation_second_phase_started = true;
		    dive_state_data->animation_start_time = game_state->time;
		}
		struct assets_animation *animation = assets_animation_get_by_id(
		    game_state->animation_cache,
		    goalie->dive_animations_end[SS_UP][direction][goalie->dive_height]
					       [dive_state_data->ball_held]);
		f32 animation_completion =
		    Clamp((game_state->time - dive_state_data->animation_start_time) /
			      (animation->delay * animation->frames_count),
			  0,
			  1);
		const u16 new_frame_id =
		    floorf(animation_completion * (animation->frames_count - 1));
		if (animation_completion >= 1) {
		    animation_completion = 1;
		    dive_state_data->animation_completed = true;
		    dive_state_data->animation_end_time = game_state->time;
		}
		sprite->frame_id = animation->frames[new_frame_id];
	    }

	    v2 force_to_apply = Vector2Zero();
	    if (dive_state_data->animation_completed) {
		// Apply friction
		force_to_apply =
		    Vector2Scale(Vector2Negate(Vector2Normalize(physics->velocity)), 50000);
	    } else {
		v2 steering = direction == SD_LEFT ? Vector2Left() : Vector2Right();
		force_to_apply = Vector2Scale(steering, 60000);
	    }
	    entities_player_apply_force(entity_id, force_to_apply, game_state);
	    entities_player_apply_velocity(entity_id, game_state);

	    const f32 ball_radius = ball_collider->colliders[0].props.circle.radius;
	    const b32 is_colliding_with_ball = entities_player_is_diving_goalie_colliding_with_ball(
		entity,
		V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
		ball_radius,
		game_state->constants.crossbar_height - ball_radius);

	    if (!dive_state_data->ball_saved &&
		HAS_TIME_PASSED(game_state->time, goalie->last_save_time, 0.1f) &&
		is_colliding_with_ball &&
		(!dive_state_data->animation_completed ||
		 // If ball height is very low we are going to make the save even when
		 // lying on the floor after the dive
		 ball_transform->virtual_height < ball_radius)) {
		// Choose whether to bounce or hold the ball
		// Definitely hold if ball velocity is lower than 54km/h
		dive_state_data->ball_held = ball_velocity < (15 * PIXELS_PER_METER) ||
					     GetRandomValue(0, 99) < 33;

		// TODO: if the ball hit the post and came back we don't want to hold
		// the ball
		if (dive_state_data->ball_held) {
		    entities_ball_held_by_goalie(
			game_state->vars.ball_entity_id, entity_id, game_state);
		} else {
		    // Bounce ball
		    v3 new_ball_velocity = Vector3Reflect(ball_physics_3d->velocity, (v3){.z = 1});
		    // new_ball_velocity = Vector3RotateByAxisAngle(
		    //     new_ball_velocity, (v3){.y = 1},
		    //     30 - 60 * GetRandomValue(0, 100) / 100.0f * DEG2RAD);

		    // TODO: position should be outside collider
		    entities_ball_set_position_and_velocity(
			game_state->vars.ball_entity_id,
			V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
			Vector3Scale(new_ball_velocity, 0.3f),
			game_state);
		    entities_ball_saved_by_goalie(
			game_state->vars.ball_entity_id, entity_id, game_state);
		}

		goalie->last_save_time = game_state->time;
		dive_state_data->ball_saved = true;
	    } else if (dive_state_data->animation_completed &&
		       HAS_TIME_PASSED(
			   game_state->time, dive_state_data->animation_end_time, 0.75f)) {
		if (dive_state_data->ball_held) {
		    entities_goalie_state_machine_change_state_to(
			goalie->state_machine, GS_HOLD_BALL, game_state);
		} else {
		    entities_goalie_state_machine_change_state_to(
			goalie->state_machine, GS_PROTECT_GOAL, game_state);
		}
	    }

	    break;
	}
	case GS_COUNT:
	    break;
	}
    }
}
