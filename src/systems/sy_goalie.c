#include "assets.h"
#include "components.h"
#include "e_player.h"
#include "entities.h"
#include "football.h"
#include "game.h"
#include "input.h"
#include "simulation.h"
#include "steering.h"
#include "types.h"
#include "macro.h"
#include "math_ext.h"
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
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

	const v3 ball_position = V2_TO_V3(ball_transform->position, ball_transform->virtual_height);
	const f32 ball_radius = ball_collider->colliders[0].props.circle.radius;
	const f32 ball_velocity = Vector3Length(ball_physics_3d->velocity);

	collider->colliders[PCT_PLAYER].props.circle.radius =
	    game_state->constants.player_control_radius;
	struct collider *goalie_collider = &collider->colliders[PCT_GOALIE];

	switch (state_machine->current_state) {
	case GS_NONE:
	    break;
	case GS_PROTECT_GOAL: {
	    if (entities_player_is_colliding_with_ball(entity,
						       PCT_GOALIE,
						       ball_position,
						       ball_radius,
						       game_state->constants.crossbar_height)) {

		// Choose whether to bounce or hold the ball
		// Definitely hold if ball velocity is lower than 54km/h
		// Sometimes hold if ball velocity is lower than 108km/h
		// Otherwise bounce it
		const b32 hold = GOALIE_HOLD_OR_BOUNCE(ball_velocity);

		if (hold) {
		    // hold ball in hands
		    entities_ball_held_by_goalie(
			game_state->vars.ball_entity_id, entity_id, game_state);
		    entities_goalie_state_machine_change_state_to(
			goalie->state_machine, GS_HOLD_BALL, game_state);

		} else {
		    v3 new_ball_velocity = Vector3Reflect(ball_physics_3d->velocity, (v3){.z = 1});

		    // Project ball position back in the past using current velocity until it's not colliding anymore
		    // TODO: this thing should be refactored as it's used in more than one place
		    v3 neg_velocity = Vector3Negate(ball_physics_3d->velocity);
		    b32 still_colliding_with_ball = true;
		    v3 where_to_place_ball =
			V2_TO_V3(ball_transform->position, ball_transform->virtual_height);
		    while (still_colliding_with_ball) {
			where_to_place_ball = Vector3Add(
			    where_to_place_ball, Vector3Scale(neg_velocity, game_state->dt));

			still_colliding_with_ball = entities_player_is_colliding_with_ball(
			    entity,
			    PCT_GOALIE,
			    where_to_place_ball,
			    ball_radius,
			    game_state->constants.crossbar_height);
		    }

		    entities_ball_set_position_and_velocity(game_state->vars.ball_entity_id,
							    where_to_place_ball,
							    Vector3Scale(new_ball_velocity, 0.3f),
							    game_state);
		    entities_ball_saved_by_goalie(
			game_state->vars.ball_entity_id, entity_id, game_state);

		    // TODO: we should play some animation here and maybe transition to some stunned state
		}

		break;
	    }

	    // So we are not colliding with ball...

	    v2 destination;
	    v2 direction;
	    v2 steering;
	    f32 force;

	    const f32 exit_max_speed = game_state->constants.player_run_max_velocity * 1.5f;

	    // can we get the ball in our hands without moving by any chance?
	    if (ball_transform->virtual_height > game_state->constants.crossbar_height &&
		ball->future_reachable.available && ball->future_reachable.dts < 20 &&
		Vector2Distance(V3_TO_V2(ball->future_reachable.position), transform->position) <
		    5) {
		entities_goalie_state_machine_change_state_to(state_machine, GS_JUMP, game_state);
		break;
	    }

	    // Calculate whether goalie can reach the ball at its next reachable position
	    const b32 should_seek_ball_at_reachable_pos =
		!ball->is_on_the_ground && ball->future_reachable.available &&
		entities_common_can_reach_position_in_time(
		    transform->position,
		    V3_TO_V2(ball->future_reachable.position),
		    game_state->dt * ball->future_bounce.dts,
		    exit_max_speed) &&
		football_pitch_is_position_inside_penalty_area(
		    V3_TO_V2(ball->future_reachable.position), player->side, game_state);

	    if (should_seek_ball_at_reachable_pos) {
		// seek ball future bounce position
		physics->max_velocity = exit_max_speed;
		force = game_state->constants.player_movement_force * 2;

		destination = V3_TO_V2(ball->future_reachable.position);
		direction = Vector2Normalize(Vector2Subtract(destination, transform->position));
		steering = steering_arrive(
		    transform->position, physics->velocity, physics->max_velocity, destination, 10);
	    } else {
		// Calculate whether goalie can reach the ball using predicted positions
		// TODO: don't rush to the ball if possession is hostile

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

	    entities_player_calculate_running_frame(entity_id,
						    Vector2Subtract(ball_transform->position,
								    transform->position),
						    5,
						    game_state);

	    if (ball_velocity > game_state->constants.goalie_dive_min_ball_velocity &&
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
	case GS_JUMP: {
	    struct state_machine_goalie_animation_data *jump_state_data =
		&goalie->state_machine->state_data.dive;

	    if (!jump_state_data->animation_first_phase_completed) {
		struct assets_animation *animation =
		    assets_animation_get_by_id(game_state->assets->animation_cache,
					       goalie->jump_animations_start[player->side]);
		f32 animation_completion =
		    Clamp((game_state->time - jump_state_data->animation_start_time) /
			      (animation->delay * animation->frames_count),
			  0,
			  1);
		const u16 new_frame_id =
		    floorf(animation_completion * (animation->frames_count - 1));
		if (animation_completion >= 1) {
		    animation_completion = 1;
		    jump_state_data->animation_first_phase_completed = true;
		    jump_state_data->animation_first_phase_end_time = game_state->time;
		}
		sprite->frame_id = animation->frames[new_frame_id];
	    } else if ((jump_state_data->ball_held ||
			HAS_TIME_PASSED(game_state->time,
					jump_state_data->animation_first_phase_end_time,
					0.05f)) &&
		       !jump_state_data->animation_completed) {
		if (!jump_state_data->animation_second_phase_started) {
		    jump_state_data->animation_second_phase_started = true;
		    jump_state_data->animation_start_time = game_state->time;
		}
		struct assets_animation *animation = assets_animation_get_by_id(
		    game_state->assets->animation_cache,
		    goalie->jump_animations_end[player->side][jump_state_data->ball_held]);
		f32 animation_completion =
		    Clamp((game_state->time - jump_state_data->animation_start_time) /
			      (animation->delay * animation->frames_count),
			  0,
			  1);
		const u16 new_frame_id =
		    floorf(animation_completion * (animation->frames_count - 1));
		if (animation_completion >= 1) {
		    animation_completion = 1;
		    jump_state_data->animation_completed = true;
		    jump_state_data->animation_end_time = game_state->time;
		}
		sprite->frame_id = animation->frames[new_frame_id];
	    }

	    const b32 is_colliding_with_ball =
		entities_player_is_colliding_with_ball(entity,
						       PCT_GOALIE,
						       ball_position,
						       ball_radius,
						       game_state->constants.crossbar_height);

	    if (!jump_state_data->ball_saved &&
		HAS_TIME_PASSED(game_state->time, goalie->last_save_time, 0.1f) &&
		is_colliding_with_ball && !jump_state_data->animation_completed) {
		// Choose whether to bounce or hold the ball
		// Definitely hold if ball velocity is lower than 54km/h
		// Sometimes hold if ball velocity is lower than 108km/h
		// Otherwise bounce it
		jump_state_data->ball_held = GOALIE_HOLD_OR_BOUNCE(ball_velocity);

		if (jump_state_data->ball_held) {
		    entities_ball_held_by_goalie(
			game_state->vars.ball_entity_id, entity_id, game_state);
		    printf("ball held\n");
		} else {
		    // Bounce ball
		    v3 new_ball_velocity =
			Vector3Normalize(Vector3Reflect(ball_physics_3d->velocity, (v3){.z = 1}));
		    new_ball_velocity.y = 1;

		    // Project ball position back in the past using current velocity until it's not colliding anymore
		    v3 neg_velocity = Vector3Negate(ball_physics_3d->velocity);
		    b32 still_colliding_with_ball = true;
		    v3 where_to_place_ball =
			V2_TO_V3(ball_transform->position, ball_transform->virtual_height);
		    while (still_colliding_with_ball) {
			where_to_place_ball = Vector3Add(
			    where_to_place_ball, Vector3Scale(neg_velocity, game_state->dt));

			still_colliding_with_ball = entities_player_is_colliding_with_ball(
			    entity,
			    PCT_GOALIE,
			    where_to_place_ball,
			    ball_radius,
			    game_state->constants.crossbar_height - ball_radius);
		    }

		    entities_ball_set_position_and_velocity(game_state->vars.ball_entity_id,
							    where_to_place_ball,
							    Vector3Scale(new_ball_velocity,
									 10 * PIXELS_PER_METER),
							    game_state);
		    entities_ball_saved_by_goalie(
			game_state->vars.ball_entity_id, entity_id, game_state);
		    printf("ball saved\n");
		}

		goalie->last_save_time = game_state->time;
		jump_state_data->ball_saved = true;
	    } else if (jump_state_data->animation_completed) {
		if (jump_state_data->ball_held) {
		    entities_goalie_state_machine_change_state_to(
			goalie->state_machine, GS_HOLD_BALL, game_state);
		} else {
		    entities_goalie_state_machine_change_state_to(
			goalie->state_machine, GS_PROTECT_GOAL, game_state);
		}
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
	    i8 direction = player->side == PSI_NORTH ? 1 : -1;
	    const v2 new_ball_position = Vector2Add(
		transform->position,
		Vector2Scale((v2){.y = direction},
			     goalie_collider->props.circle.offset.y +
				 goalie_collider->props.circle.radius + ball_radius + 7));
	    entities_ball_set_position_and_velocity(game_state->vars.ball_entity_id,
						    V2_TO_V3(new_ball_position, 10),
						    Vector3Zero(),
						    game_state);
	    entities_ball_long_pass(
		game_state->vars.ball_entity_id, entity_id, (v2){.y = direction}, 0.7f, game_state);
	    entities_goalie_state_machine_change_state_to(
		goalie->state_machine, GS_PROTECT_GOAL, game_state);
	    break;
	}
	case GS_DIVE: {
	    enum save_direction direction = goalie->dive_direction;

	    struct state_machine_goalie_animation_data *dive_state_data =
		&goalie->state_machine->state_data.dive;

	    if (!dive_state_data->animation_first_phase_completed) {
		struct assets_animation *animation = assets_animation_get_by_id(
		    game_state->assets->animation_cache,
		    goalie->dive_animations_start[player->side][direction][goalie->dive_height]);
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
		    game_state->assets->animation_cache,
		    goalie->dive_animations_end[player->side][direction][goalie->dive_height]
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
		force_to_apply = Vector2Scale(Vector2Negate(Vector2Normalize(physics->velocity)),
					      50000); // TODO: this should depend on type of pitch
	    } else {
		v2 steering = direction == SD_LEFT ? Vector2Left() : Vector2Right();
		force_to_apply = Vector2Scale(steering, 60000);
	    }
	    entities_player_apply_force(entity_id, force_to_apply, game_state);
	    entities_player_apply_velocity(entity_id, game_state);

	    const b32 is_colliding_with_ball = entities_player_is_colliding_with_ball(
		entity,
		PCT_DIVE,
		ball_position,
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
		// Sometimes hold if ball velocity is lower than 108km/h
		// Otherwise bounce it
		dive_state_data->ball_held =
		    GOALIE_HOLD_OR_BOUNCE(ball_velocity) &&
		    ((player->side == PSI_NORTH && ball_physics_3d->velocity.z < 0) ||
		     (player->side == PSI_SOUTH && ball_physics_3d->velocity.z > 0));

		// TODO: if the ball hits the post and comes back we don't want to hold
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

		    // Project ball position back in the past using current velocity until it's not colliding anymore
		    v3 neg_velocity = Vector3Negate(ball_physics_3d->velocity);
		    b32 still_colliding_with_ball = true;
		    v3 where_to_place_ball =
			V2_TO_V3(ball_transform->position, ball_transform->virtual_height);
		    while (still_colliding_with_ball) {
			where_to_place_ball = Vector3Add(
			    where_to_place_ball, Vector3Scale(neg_velocity, game_state->dt));

			still_colliding_with_ball = entities_player_is_colliding_with_ball(
			    entity,
			    PCT_DIVE,
			    where_to_place_ball,
			    ball_radius,
			    game_state->constants.crossbar_height - ball_radius);
		    }

		    entities_ball_set_position_and_velocity(game_state->vars.ball_entity_id,
							    where_to_place_ball,
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

	layers_debug_queue_draw_text(
	    game_state->layer_debug,
	    TextFormat("%-8s %02d pos: [%7.2f, %7.2f] vel: %5.2f state: %s",
		       "Goalie",
		       entity_id,
		       transform->position.x,
		       transform->position.y,
		       VELOCITY_TO_KMH(Vector2Length(physics->velocity)),
		       state_machine_goalie_state_string[goalie->state_machine->current_state]));
    }
}
