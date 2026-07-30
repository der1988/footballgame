#include "entities/entities.h"
#include "entities/e_ball.h"
#include "events/events.h"
#include "game/game.h"
#include "log/log.h"
#include "macro.h"
#include "math_ext/math_ext.h"
#include "collision.h"
#include <raymath.h>

void systems_goal(struct game_state *game_state)
{
	struct entity *ball_entity = game_state->vars.ball.entity;

	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);

	UNUSED(ball);

	const f32 ball_radius = ball_collider->colliders[0].props.circle.radius;
	const f32 post_width = 2;

	// First let's check if the entity is a goal
	for (usize i = 0; i < PSI_COUNT; ++i) {
		struct entity *entity = game_state->vars.pitch.goal[i].entity;

		DECLARE_ENTITY_COMPONENT(entity, transform);
		DECLARE_ENTITY_COMPONENT(entity, goal);
		DECLARE_ENTITY_COMPONENT(entity, collider);

		const rect_t goal_rectangle =
			calculate_collision_rectangle(transform->position,
										  collider->colliders[0].props.rectangle.size,
										  collider->colliders[0].props.rectangle.offset);

		if (game_state->vars.match.state == MS_GOAL) {
			// If we scored we have to apply special physics to the ball to make it stop inside the goal itself
			if (((i == PSI_NORTH && ball_transform->position.y < goal_rectangle.y - 3) ||
				 (i == PSI_SOUTH && ball_transform->position.y >
										(goal_rectangle.y + goal_rectangle.height * 2 + 3))) ||
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
				   CheckCollisionCircleRec(ball_transform->position, ball_radius, goal_rectangle) &&
				   ((i == PSI_NORTH &&
					 ball_transform->position.y < game_state->vars.pitch.goal[i].center.y &&
					 ball_physics_3d->velocity.z < 0) ||
					(i == PSI_SOUTH &&
					 ball_transform->position.y > game_state->vars.pitch.goal[i].center.y &&
					 ball_physics_3d->velocity.z > 0))) {
			// If the ball is within the goal rectangle and over the goal line then we can check whether we scored
			// or we hit the posts. The check is in 3 dimensions, because the ball physics is in 3D.

			v3 current_pos = V2_TO_V3(ball_transform->position, ball_transform->virtual_height);

			// We negate current velocity to build a ray that we can use to check collision against a 3D quad with the
			// dimensions of the goal.
			// It's like projecting the trajectory of the ball back into the field and checking where it collides the goal.
			ray_t ray = (ray_t){.position = current_pos,
								.direction = Vector3Negate(ball_physics_3d->velocity)};
			v3 *goal_quad_points = game_state->vars.pitch.goal[goal->side].goal_quad_points;
			ray_collision_t collision = GetRayCollisionQuad(ray,
															goal_quad_points[0],
															goal_quad_points[1],
															goal_quad_points[2],
															goal_quad_points[3]);
			// printf("original collision %.2f, %.2f, %.2f\n", collision.point.x,
			//        collision.point.y, collision.point.z);
			b32 hit_goal = collision.hit && collision.point.x > (goal_rectangle.x - ball_radius) &&
						   collision.point.x <
							   (goal_rectangle.x + goal_rectangle.width + ball_radius) &&
						   collision.point.y < (goal_rectangle.height + ball_radius);

			if (!hit_goal) {
				continue;
			}

			// If we reached this point we hit the goal in a specific position: now let's check if we hit a post or we scored.

			b32 collided_with_post = false;
			b32 scored = false;
			v3 new_velocity = ball_physics_3d->velocity;

			if (HAS_TIME_PASSED(game_state->time, goal->hit_post_time, 0.05f) &&
				collision.point.x < (goal_rectangle.x + post_width + ball_radius)) {

				// Since we are already sure we hit the goal, if the collision x axis is lower than (post width + ball radius) then we hit the left post.
				//                  | |
				//    LEFT POST ->  | |*  <--- BALL
				//                  | |

				// We normalise the hit point on the post between -1 and +1, where zero means we fully hit the post
				f32 normalised_hit_point =
					Clamp((collision.point.x - goal_rectangle.x - post_width * 0.5f) / post_width,
						  -1,
						  1) *
					(i == PSI_NORTH ? 1 : -1);

				// We invert the y axis of the velocity of the ball and rotate the vector based on the normalised hit point.
				// This gives us a realistic bounce against the post.
				// The rotation is on the y axis, because the ball will be deflected left or right (or it will come back to us if we hit the post squarely).
				new_velocity.y *= -1;
				new_velocity = Vector3RotateByAxisAngle(Vector3Negate(new_velocity),
														(v3){.y = 1},
														120 * normalised_hit_point * DEG2RAD);
				collided_with_post = true;
				goal->hit_post_time = game_state->time;

				GLOGF_TRACE(LS_GAME, "hit left post: %.2f", normalised_hit_point);

				// If the resulting velocity is towards the goal we scored.
				if (new_velocity.x > 0 && ((i == PSI_NORTH && new_velocity.z < 0) ||
										   (i == PSI_SOUTH && new_velocity.z > 0))) {
					scored = true;
				}
			} else if (HAS_TIME_PASSED(game_state->time, goal->hit_post_time, 0.05f) &&
					   collision.point.x >
						   (goal_rectangle.x + goal_rectangle.width - post_width - ball_radius)) {

				// Since we are already sure we hit the goal, if the collision x axis is greater than (post width - ball radius) then we hit the right post.
				//              | |
				//    BALL ->  *| |  <--- RIGHT POST
				//              | |

				f32 normalised_hit_point = Clamp((collision.point.x - goal_rectangle.x -
												  goal_rectangle.width + post_width * 0.5f) /
													 post_width,
												 -1,
												 1) *
										   (i == PSI_NORTH ? 1 : -1);

				new_velocity.y *= -1;
				new_velocity = Vector3RotateByAxisAngle(Vector3Negate(new_velocity),
														(v3){.y = 1},
														120 * normalised_hit_point * DEG2RAD);
				collided_with_post = true;
				goal->hit_post_time = game_state->time;

				GLOGF_TRACE(LS_GAME, "hit right post: %.2f", normalised_hit_point);

				// If the resulting velocity is towards the goal we scored.
				if (new_velocity.x < 0 && ((i == PSI_NORTH && new_velocity.z < 0) ||
										   (i == PSI_SOUTH && new_velocity.z > 0))) {
					scored = true;
				}
			} else if (HAS_TIME_PASSED(game_state->time, goal->hit_post_time, 0.05f) &&
					   collision.point.y > (goal_rectangle.height - post_width - ball_radius)) {

				// Since we are already sure we hit the goal, if the collision y axis is lower than (post width + ball radius) then we hit the crossbar.
				//               __
				//     BALL --> /  \
				// ---------------------------------
				// ---------------------------------
				//            CROSSBAR

				f32 normalised_hit_point =
					Clamp((collision.point.y - goal_rectangle.height + post_width) / post_width,
						  -1,
						  1) *
					(i == PSI_NORTH ? -1 : 1);

				// Here the rotation is on the x axis, because the ball will be deflected up or down.
				new_velocity.x *= -1;
				new_velocity = Vector3RotateByAxisAngle(Vector3Negate(new_velocity),
														(v3){.x = 1},
														120 * normalised_hit_point * DEG2RAD);
				collided_with_post = true;
				goal->hit_post_time = game_state->time;

				GLOGF_TRACE(LS_GAME, "hit crossbar: %.2f", normalised_hit_point);

				// If the resulting velocity is towards the goal we scored.
				if (new_velocity.y < 0 && ((i == PSI_NORTH && new_velocity.z < 0) ||
										   (i == PSI_SOUTH && new_velocity.z > 0))) {
					scored = true;
				}
			}

			if (collided_with_post) {
				new_velocity =
					Vector3Scale(new_velocity, game_state->constants.ball_cr_against_post);

				v3 new_position =
					Vector3Add(collision.point, Vector3Scale(new_velocity, game_state->dt));

				entity_ball_set_position_and_velocity(ball_entity, new_position, new_velocity);
				GLOGF_INFO(LS_GAME,
						   "hit post at %.2f, %.2f, %.2f",
						   collision.point.x,
						   collision.point.y,
						   collision.point.z);
				if (scored) {
					GLOG_INFO(LS_GAME, "goal off post!");
				}
			} else if (collision.point.x > (goal_rectangle.x + post_width + ball_radius) &&
					   collision.point.x <
						   (goal_rectangle.x + goal_rectangle.width - post_width - ball_radius) &&
					   collision.point.y < (goal_rectangle.height - post_width - ball_radius)) {
				scored = true;
			}

			if (scored) {
				events_router_publish(game_state->events_router, EVT_GOAL_SCORED, game_state);
				GLOGF_INFO(LS_GAME,
						   "goal at %.2f, %.2f, %.2f",
						   collision.point.x,
						   collision.point.y,
						   collision.point.z);
			}
		}
	}
}
