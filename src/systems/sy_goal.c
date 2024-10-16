#include "components.h"
#include "entities.h"
#include "football.h"
#include "game.h"
#include "macro.h"
#include "log.h"
#include "math_ext.h"
#include <raylib.h>
#include <raymath.h>

void systems_goal(struct game_state *game_state)
{
    struct entity *ball_entity =
	entities_entity_get(game_state->entity_repository, game_state->vars.ball_entity_id);

    declare_ball_entity_components(ball_entity);

    // First let's check if it's a goal
    for_each_entity(entity, game_state->entity_repository) {
	if (!entities_is_entity_alive_and_has_components(
		entity, 4, C_TRANSFORM, C_SPRITE, C_GOAL, C_COLLIDER)) {
	    continue;
	}

	declare_entity_component(entity, transform);
	declare_entity_component(entity, goal);
	declare_entity_component(entity, collider);

	Rectangle goal_rectangle = {
	    .x = transform->position.x + collider->colliders[0].props.rectangle.offset.x,
	    .y = transform->position.y + collider->colliders[0].props.rectangle.offset.y,
	    .width = collider->colliders[0].props.rectangle.size.x,
	    .height = collider->colliders[0].props.rectangle.size.y};

	v3 goal_line_p1 =
	    (v3){.x = game_state->vars.pitch.field.x, .y = 0, .z = game_state->vars.pitch.field.y};
	v3 goal_line_p2 = (v3){.x = game_state->vars.pitch.field.x,
			       .y = goal_rectangle.height,
			       .z = game_state->vars.pitch.field.y};
	v3 goal_line_p3 =
	    (v3){.x = game_state->vars.pitch.field.x + game_state->vars.pitch.field.width,
		 .y = goal_rectangle.height,
		 .z = game_state->vars.pitch.field.y};
	v3 goal_line_p4 =
	    (v3){.x = game_state->vars.pitch.field.x + game_state->vars.pitch.field.width,
		 .y = 0,
		 .z = game_state->vars.pitch.field.y};

	f32 post_width = 2;

	if (game_state->vars.match.state == MS_GOAL) {
	    if (ball_transform->position.y < goal_rectangle.y - 3 ||
		ball_transform->position.x < goal_rectangle.x ||
		ball_transform->position.x > (goal_rectangle.x + goal_rectangle.width)) {
		ball_physics_3d->velocity.x *= 0.8f;
		ball_physics_3d->velocity.z *= 0.8f;
		if (ball_transform->virtual_height > goal_rectangle.height &&
		    ball_physics_3d->velocity.y > 0) {
		    ball_physics_3d->velocity.y *= 0.9f;
		}
	    }
	} else if (game_state->vars.match.state == MS_PLAY &&
		   CheckCollisionCircleRec(ball_transform->position,
					   ball_collider->colliders[0].props.circle.radius,
					   goal_rectangle) &&
		   ball_transform->position.y < game_state->vars.pitch.field.y) {

	    v3 current_pos = V2_TO_V3(ball_transform->position, ball_transform->virtual_height);

	    Ray ray = (Ray){.position = current_pos,
			    .direction = Vector3Negate(ball_physics_3d->velocity)};
	    RayCollision collision =
		GetRayCollisionQuad(ray, goal_line_p1, goal_line_p2, goal_line_p3, goal_line_p4);
	    // printf("original collision %.2f, %.2f, %.2f\n", collision.point.x,
	    //        collision.point.y, collision.point.z);
	    if (collision.hit &&
		collision.point.x >
		    (goal_rectangle.x - ball_collider->colliders[0].props.circle.radius) &&
		collision.point.x < (goal_rectangle.x + goal_rectangle.width +
				     ball_collider->colliders[0].props.circle.radius) &&
		collision.point.y <
		    (goal_rectangle.height + ball_collider->colliders[0].props.circle.radius)) {

		b32 collided_with_post = false;
		b32 scored = false;
		v3 new_velocity = ball_physics_3d->velocity;

		if (HAS_TIME_PASSED(game_state->time, goal->hit_left_post_time, 0.1f) &&
		    collision.point.x < (goal_rectangle.x + post_width +
					 ball_collider->colliders[0].props.circle.radius)) {
		    // hit left post
		    f32 normalised_hit_point = Clamp(
			(collision.point.x - goal_rectangle.x - post_width * 0.5f) / post_width,
			-1,
			1);
		    new_velocity.y *= -1;
		    new_velocity = Vector3RotateByAxisAngle(Vector3Negate(new_velocity),
							    (v3){.y = 1},
							    120 * normalised_hit_point * DEG2RAD);
		    collided_with_post = true;
		    goal->hit_left_post_time = game_state->time;
		    // This needs to be generalised to account for goal position (top vs
		    // bottom)
		    if (new_velocity.x > 0 && new_velocity.z < 0) {
			scored = true;
		    }
		} else if (HAS_TIME_PASSED(game_state->time, goal->hit_right_post_time, 0.1f) &&
			   collision.point.x >
			       (goal_rectangle.x + goal_rectangle.width - post_width -
				ball_collider->colliders[0].props.circle.radius)) {
		    // hit right post
		    f32 normalised_hit_point = Clamp((collision.point.x - goal_rectangle.x -
						      goal_rectangle.width + post_width * 0.5f) /
							 post_width,
						     -1,
						     1);
		    new_velocity.y *= -1;
		    new_velocity = Vector3RotateByAxisAngle(Vector3Negate(new_velocity),
							    (v3){.y = 1},
							    120 * normalised_hit_point * DEG2RAD);
		    collided_with_post = true;
		    goal->hit_right_post_time = game_state->time;
		    if (new_velocity.x < 0 && new_velocity.z < 0) {
			scored = true;
		    }
		} else if (HAS_TIME_PASSED(game_state->time, goal->hit_crossbar_time, 0.1f) &&
			   collision.point.y > (goal_rectangle.height - post_width -
						ball_collider->colliders[0].props.circle.radius)) {
		    // hit cross bar
		    f32 normalised_hit_point =
			Clamp((collision.point.y - goal_rectangle.height + post_width) / post_width,
			      -1,
			      1) *
			-1;
		    new_velocity.x *= -1;
		    new_velocity = Vector3RotateByAxisAngle(Vector3Negate(new_velocity),
							    (v3){.x = 1},
							    120 * normalised_hit_point * DEG2RAD);
		    collided_with_post = true;
		    goal->hit_crossbar_time = game_state->time;
		    if (new_velocity.z < 0 && new_velocity.y < 0) {
			scored = true;
		    }
		}
		if (collided_with_post) {
		    ball->subject_to_after_touch = false;
		    new_velocity =
			Vector3Scale(new_velocity, game_state->constants.ball_cr_against_post);

		    v3 new_position =
			Vector3Add(collision.point, Vector3Scale(new_velocity, game_state->dt));

		    entities_ball_set_position_and_velocity(
			game_state->vars.ball_entity_id, new_position, new_velocity, game_state);
		    GLOGF_INFO(LS_GAME,
			       "hit post at %.2f, %.2f, %.2f",
			       collision.point.x,
			       collision.point.y,
			       collision.point.z);
		    if (scored) {
			GLOG_INFO(LS_GAME, "goal off post!");
		    }
		} else if (collision.point.x > (goal_rectangle.x + post_width +
						ball_collider->colliders[0].props.circle.radius) &&
			   collision.point.x <
			       (goal_rectangle.x + goal_rectangle.width - post_width -
				ball_collider->colliders[0].props.circle.radius) &&
			   collision.point.y < (goal_rectangle.height - post_width -
						ball_collider->colliders[0].props.circle.radius)) {
		    scored = true;
		}
		if (scored) {
		    entities_ball_in_goal(game_state->vars.ball_entity_id, game_state);
		    football_match_change_state(game_state, MS_GOAL);
		    GLOGF_INFO(LS_GAME,
			       "goal at %.2f, %.2f, %.2f",
			       collision.point.x,
			       collision.point.y,
			       collision.point.z);
		}
	    }
	}
	if (game_state->vars.match.state == MS_PLAY &&
	    !CheckCollisionPointRec(ball_transform->position, game_state->vars.pitch.field)) {
	    // Ball is out of bounds
	    Rectangle *pitch_rectangle = &game_state->vars.pitch.field;
	    if (ball_transform->position.x < pitch_rectangle->x - 50) {
		// Throw in left
		football_match_change_state(game_state, MS_THROW_IN);
	    } else if (ball_transform->position.x >
		       (pitch_rectangle->x + pitch_rectangle->width + 50)) {
		// Throw in right
		football_match_change_state(game_state, MS_THROW_IN);
	    } else if (ball_transform->position.y < pitch_rectangle->y - 50) {
		// Corner kick north
		football_match_change_state(game_state, MS_CORNER_KICK);
	    } else if (ball_transform->position.y >
		       (pitch_rectangle->y + pitch_rectangle->height + 50)) {
		// Corner kick south
		football_match_change_state(game_state, MS_CORNER_KICK);
	    }
	}
    }
}
