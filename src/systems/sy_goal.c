#include "entities.h"
#include "e_ball.h"
#include "events/events.h"
#include "game.h"
#include "log.h"
#include "math_ext.h"
#include "collision.h"
#include <raymath.h>
#include <stdio.h>

void systems_goal(struct game_state *game_state)
{
	struct entity *ball_entity = game_state->vars.ball_entity;

	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);

	const f32 ball_radius = ball_collider->colliders[0].props.circle.radius;
	const f32 post_width = 2;

	// First let's check if it's a goal
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
					 ball_transform->position.y < game_state->vars.pitch.goal[i].center.y) ||
					(i == PSI_SOUTH &&
					 ball_transform->position.y > game_state->vars.pitch.goal[i].center.y))) {

			v3 current_pos = V2_TO_V3(ball_transform->position, ball_transform->virtual_height);

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

			b32 collided_with_post = false;
			b32 scored = false;
			v3 new_velocity = ball_physics_3d->velocity;

			if (HAS_TIME_PASSED(game_state->time, goal->hit_left_post_time, 0.1f) &&
				collision.point.x < (goal_rectangle.x + post_width + ball_radius)) {
				// hit left post
				f32 normalised_hit_point =
					Clamp((collision.point.x - goal_rectangle.x - post_width * 0.5f) / post_width,
						  -1,
						  1) *
					(i == PSI_NORTH ? 1 : -1);
				new_velocity.y *= -1;
				new_velocity = Vector3RotateByAxisAngle(Vector3Negate(new_velocity),
														(v3){.y = 1},
														120 * normalised_hit_point * DEG2RAD);
				collided_with_post = true;
				goal->hit_left_post_time = game_state->time;

				if (new_velocity.x > 0 && ((i == PSI_NORTH && new_velocity.z < 0) ||
										   (i == PSI_SOUTH && new_velocity.z > 0))) {
					scored = true;
				}
			} else if (HAS_TIME_PASSED(game_state->time, goal->hit_right_post_time, 0.1f) &&
					   collision.point.x >
						   (goal_rectangle.x + goal_rectangle.width - post_width - ball_radius)) {
				// hit right post
				f32 normalised_hit_point = Clamp((collision.point.x - goal_rectangle.x -
												  goal_rectangle.width + post_width * 0.5f) /
													 post_width,
												 -1,
												 1) *
										   (i == PSI_NORTH ? 1 : -1);
				;
				new_velocity.y *= -1;
				new_velocity = Vector3RotateByAxisAngle(Vector3Negate(new_velocity),
														(v3){.y = 1},
														120 * normalised_hit_point * DEG2RAD);
				collided_with_post = true;
				goal->hit_right_post_time = game_state->time;

				if (new_velocity.x < 0 && ((i == PSI_NORTH && new_velocity.z < 0) ||
										   (i == PSI_SOUTH && new_velocity.z > 0))) {
					scored = true;
				}
			} else if (HAS_TIME_PASSED(game_state->time, goal->hit_crossbar_time, 0.1f) &&
					   collision.point.y > (goal_rectangle.height - post_width - ball_radius)) {
				// hit cross bar
				f32 normalised_hit_point =
					Clamp((collision.point.y - goal_rectangle.height + post_width) / post_width,
						  -1,
						  1) *
					(i == PSI_NORTH ? -1 : 1);
				;
				new_velocity.x *= -1;
				new_velocity = Vector3RotateByAxisAngle(Vector3Negate(new_velocity),
														(v3){.x = 1},
														120 * normalised_hit_point * DEG2RAD);
				collided_with_post = true;
				goal->hit_crossbar_time = game_state->time;

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

	// TODO: move this thing in a proper location
	if (game_state->vars.match.state == MS_PLAY &&
		!CheckCollisionPointRec(ball_transform->position, game_state->vars.pitch.field)) {
		// Ball is out of bounds
		rect_t *pitch_rectangle = &game_state->vars.pitch.field;
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
