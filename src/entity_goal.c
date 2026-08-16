#include "entity.h"
#include "entity_ball.h"
#include "entity_goal.h"
#include "entity_goal_net.h"
#include "football.h"
#include "log.h"
#include "collision.h"
#include "macro.h"
#include "math_ext.h"
#include <assert.h>
#include <raymath.h>
#include "game.h"

#define GOAL_POST_WIDTH		   2
#define GOAL_HIT_POST_COOLDOWN 0.05f
// How far off a squarely struck post the deflection can be rotated, in degrees.
#define GOAL_DEFLECTION_ANGLE  120

static const char *goal_woodwork_name[] = {"none", "left post", "right post", "crossbar"};

static struct goal_collision goal_trajectory_impact(rect_t goal_rectangle,
													f32 ball_radius,
													v3 ball_position,
													v3 ball_velocity,
													const v3 *goal_quad_points);
static v3 goal_deflection_velocity(v3 velocity, v3 axis, f32 normalised_hit_point);

struct entity *entity_goal_create(struct goal_config config, struct game_state *game_state)
{
	struct entity *entity = entity_init(game_state->entity_repository, ET_GOAL, "goal_%02d");

	// Initialise transform
	struct component_transform *transform = &entity->transform;
	transform->header.enabled = true;
	transform->rotation = 0;
	transform->virtual_height = 0;
	transform->scale = (v2){.x = 1, .y = 1};
	transform->position = Vector2Add(config.center, (v2){.x = 0, .y = 12});

	// Create goal net
	struct entity *goal_net_entity = entity_goal_net_create(config, game_state);
	struct component_transform *goal_net_transform = &goal_net_entity->transform;
	goal_net_transform->parent = transform;
	goal_net_transform->position = (v2){.x = 0, .y = config.side == PSI_NORTH ? -18 : 14};

	// Initialise sprite
	struct component_sprite *sprite = &entity->sprite;
	sprite->header.enabled = true;
	char *posts_frame_name = "posts_0";
	struct asset_sprite_frame *sprite_frame =
		asset_sprite_frame_get_by_name(game_state->assets.sprite_frame_cache, posts_frame_name);
	assert(sprite_frame != NULL);
	sprite->frame_id = sprite_frame->id;
	sprite->color = WHITE;
	sprite->flipX = false;
	sprite->flipY = false;
	sprite->pivot_override = (v2){.x = 0.5, .y = 1};
	sprite->layer = config.side == PSI_NORTH ? SL_NORTH_GOAL : SL_SOUTH_GOAL;
	sprite->draw_shadow = false;
	sprite->shadow_color = BLACK;
	sprite->shadow_color.a = 100;
	sprite->shadow_offset = (v2){.x = 0.05, .y = 0.1};
	sprite->shadow_scale = (v2){.x = 1, .y = 1};

	// Initialise collider
	struct component_collider *collider = &entity->collider;
	collider->header.enabled = true;
	collider->colliders =
		memory_arena_push(game_state->memory_by_lifetime[MAL_LEVEL], sizeof(struct collider));
	assert(collider->colliders);
	collider->num_colliders = 1;
	collider->colliders[0].shape = CS_RECTANGLE;
	collider->colliders[0].props.rectangle.size =
		(v2){.x = game_state->vars.pitch.goal_width, .y = game_state->vars.pitch.goal_height};
	collider->colliders[0].props.rectangle.offset =
		(v2){.x = 0,
			 .y = config.side == PSI_NORTH ? -collider->colliders[0].props.rectangle.size.y + 5 :
											 -collider->colliders[0].props.rectangle.size.y + 4};

	// Initialise goal
	struct component_goal *goal = &entity->goal;
	goal->header.enabled = true;
	goal->side = config.side;

	GLOG_INFO_CTX(LS_ENTITIES, entity->header.name, "goal created");

	return entity;
}

rect_t entity_goal_rectangle(struct entity *goal_entity)
{
	DECLARE_ENTITY_COMPONENT(goal_entity, transform);
	DECLARE_ENTITY_COMPONENT(goal_entity, collider);

	return collision_calculate_rectangle(transform->position,
										 collider->colliders[0].props.rectangle.size,
										 collider->colliders[0].props.rectangle.offset);
}

struct goal_collision entity_goal_resolve_ball_impact(struct entity *goal_entity,
													  struct entity *ball_entity,
													  struct game_state *game_state)
{
	DECLARE_ENTITY_COMPONENT(goal_entity, goal);
	DECLARE_BALL_ENTITY_COMPONENTS(ball_entity);

	UNUSED(ball);

	const f32 ball_radius = ball_collider->colliders[0].props.circle.radius;
	const rect_t goal_rectangle = entity_goal_rectangle(goal_entity);
	const enum pitch_side side = goal->side;
	const struct goal *pitch_goal = &game_state->vars.pitch.goal[side];

	// The ball has to be inside the frame and past the line, still travelling towards the net.
	b32 over_the_line = side == PSI_NORTH ?
							(ball_transform->position.y < pitch_goal->center.y &&
							 ball_physics_3d->velocity.z < 0) :
							(ball_transform->position.y > pitch_goal->center.y &&
							 ball_physics_3d->velocity.z > 0);
	if (!over_the_line ||
		!CheckCollisionCircleRec(ball_transform->position, ball_radius, goal_rectangle)) {
		return (struct goal_collision){.hit = false};
	}

	struct goal_collision result =
		goal_trajectory_impact(goal_rectangle,
							   ball_radius,
							   V2_TO_V3(ball_transform->position, ball_transform->virtual_height),
							   ball_physics_3d->velocity,
							   pitch_goal->goal_quad_points);
	if (!result.hit) {
		return result;
	}

	const f32 mouth_left = goal_rectangle.x + GOAL_POST_WIDTH + ball_radius;
	const f32 mouth_right =
		goal_rectangle.x + goal_rectangle.width - GOAL_POST_WIDTH - ball_radius;
	const f32 mouth_top = goal_rectangle.height - GOAL_POST_WIDTH - ball_radius;
	const b32 cooldown_elapsed =
		HAS_TIME_PASSED(game_state->time, goal->hit_post_time, GOAL_HIT_POST_COOLDOWN);

	// The rotation axis is Y for the posts (the ball is deflected left or right) and X for the
	// crossbar (up or down). The hit point is normalised between -1 and +1 across the member,
	// where zero means it was struck squarely, and its sign flips with the pitch side.
	f32 normalised_hit_point = 0;
	v3 axis = {0};

	if (cooldown_elapsed && result.point.x < mouth_left) {
		//                  | |
		//    LEFT POST ->  | |*  <--- BALL
		//                  | |
		result.woodwork = GW_LEFT_POST;
		axis = (v3){.y = 1};
		normalised_hit_point =
			Clamp((result.point.x - goal_rectangle.x - GOAL_POST_WIDTH * 0.5f) / GOAL_POST_WIDTH,
				  -1,
				  1) *
			(side == PSI_NORTH ? 1 : -1);
	} else if (cooldown_elapsed && result.point.x > mouth_right) {
		//              | |
		//    BALL ->  *| |  <--- RIGHT POST
		//              | |
		result.woodwork = GW_RIGHT_POST;
		axis = (v3){.y = 1};
		normalised_hit_point = Clamp((result.point.x - goal_rectangle.x - goal_rectangle.width +
									  GOAL_POST_WIDTH * 0.5f) /
										 GOAL_POST_WIDTH,
									 -1,
									 1) *
							   (side == PSI_NORTH ? 1 : -1);
	} else if (cooldown_elapsed && result.point.y > mouth_top) {
		//               __
		//     BALL --> /  \
		// ---------------------------------
		// ---------------------------------
		//            CROSSBAR
		result.woodwork = GW_CROSSBAR;
		axis = (v3){.x = 1};
		normalised_hit_point =
			Clamp((result.point.y - goal_rectangle.height + GOAL_POST_WIDTH) / GOAL_POST_WIDTH,
				  -1,
				  1) *
			(side == PSI_NORTH ? -1 : 1);
	}

	// While the cooldown holds, no woodwork is reported at all: a ball still inside the frame
	// from the previous bounce is judged on the mouth test alone.
	if (result.woodwork == GW_NONE) {
		result.scored = result.point.x > mouth_left && result.point.x < mouth_right &&
						result.point.y < mouth_top;
		return result;
	}

	GLOGF_TRACE(LS_GAME,
				"hit %s: %.2f",
				goal_woodwork_name[result.woodwork],
				normalised_hit_point);

	v3 deflected = goal_deflection_velocity(ball_physics_3d->velocity, axis, normalised_hit_point);

	// A post has to send the ball goalwards along x, the crossbar along y, and either way it
	// must still be crossing the line.
	b32 still_going_in = result.woodwork == GW_CROSSBAR	 ? deflected.y < 0 :
						 result.woodwork == GW_LEFT_POST ? deflected.x > 0 :
														   deflected.x < 0;
	result.scored =
		still_going_in && (side == PSI_NORTH ? deflected.z < 0 : deflected.z > 0);

	deflected = Vector3Scale(deflected, game_state->constants.ball_cr_against_post);
	result.deflection.velocity = deflected;
	result.deflection.position =
		Vector3Add(result.point, Vector3Scale(deflected, game_state->dt));

	return result;
}

// Projects the ball's trajectory back into the field and returns where it met the goal plane.
static struct goal_collision goal_trajectory_impact(rect_t goal_rectangle,
												   f32 ball_radius,
												   v3 ball_position,
												   v3 ball_velocity,
												   const v3 *goal_quad_points)
{
	ray_t ray = (ray_t){.position = ball_position, .direction = Vector3Negate(ball_velocity)};
	ray_collision_t collision = GetRayCollisionQuad(ray,
													goal_quad_points[0],
													goal_quad_points[1],
													goal_quad_points[2],
													goal_quad_points[3]);

	b32 hit = collision.hit && collision.point.x > (goal_rectangle.x - ball_radius) &&
			  collision.point.x < (goal_rectangle.x + goal_rectangle.width + ball_radius) &&
			  collision.point.y < (goal_rectangle.height + ball_radius);

	return (struct goal_collision){.hit = hit, .point = collision.point, .woodwork = GW_NONE};
}

static v3 goal_deflection_velocity(v3 velocity, v3 axis, f32 normalised_hit_point)
{
	// The velocity component along the rotation axis is inverted and the vector sent back the
	// way it came, rotated by how squarely the woodwork was struck: dead centre returns the ball
	// to the shooter, an edge glances it away.
	v3 inverted = Vector3Reflect(velocity, axis);
	return Vector3RotateByAxisAngle(Vector3Negate(inverted),
									axis,
									GOAL_DEFLECTION_ANGLE * normalised_hit_point * DEG2RAD);
}
