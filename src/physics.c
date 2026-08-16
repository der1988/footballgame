#include "physics.h"
#include "entity_components.h"
#include "game.h"
#include "types.h"
#include <raymath.h>

void physics_apply_force(struct component_physics *physics, v2 force, delta_time_t dt)
{
	physics->accel = Vector2Scale(force, 1 / physics->mass);
	physics->velocity = Vector2Add(physics->velocity, Vector2Scale(physics->accel, dt));
}

void physics_clamp_velocity_to_max(struct component_physics *physics)
{
	if (Vector2Length(physics->velocity) > physics->max_velocity) {
		physics->velocity =
			Vector2Scale(Vector2Normalize(physics->velocity), physics->max_velocity);
	}
}

void physics_apply_velocity(struct component_transform *transform,
							struct component_physics *physics,
							delta_time_t dt)
{
	transform->position = Vector2Add(transform->position, Vector2Scale(physics->velocity, dt));
}

v2 physics_drag_force(v2 velocity, f32 magnitude)
{
	return Vector2Scale(Vector2Negate(Vector2Normalize(velocity)), magnitude);
}

b32 physics_set_at_rest_if_needed(struct component_physics *physics, f32 stop_speed)
{
	if (!physics_is_at_rest(physics->velocity, stop_speed)) {
		return false;
	}

	physics->velocity = Vector2Zero();
	return true;
}

f32 physics_resolve_body_collision(struct component_transform *transform_a,
								   struct component_physics *physics_a,
								   f32 radius_a,
								   struct component_transform *transform_b,
								   struct component_physics *physics_b,
								   f32 radius_b,
								   f32 bounce_factor)
{
	v2 delta = Vector2Subtract(transform_b->position, transform_a->position);
	f32 dist = Vector2Length(delta);
	f32 min_dist = radius_a + radius_b;

	if (dist >= min_dist || dist < 0.001f) {
		return 0;
	}

	v2 normal = Vector2Scale(delta, 1.0f / dist);
	f32 overlap = min_dist - dist;

	// Separation weighted by inverse mass
	f32 total_mass = physics_a->mass + physics_b->mass;
	f32 ratio_a = physics_b->mass / total_mass;
	f32 ratio_b = physics_a->mass / total_mass;

	transform_a->position =
		Vector2Subtract(transform_a->position, Vector2Scale(normal, overlap * ratio_a));
	transform_b->position =
		Vector2Add(transform_b->position, Vector2Scale(normal, overlap * ratio_b));

	// Velocity impulse when the bodies approach each other
	f32 relative_vel =
		Vector2DotProduct(Vector2Subtract(physics_a->velocity, physics_b->velocity), normal);
	if (relative_vel <= 0) {
		return 0;
	}

	f32 impulse = relative_vel * (1.0f + bounce_factor) / total_mass;
	physics_a->velocity =
		Vector2Subtract(physics_a->velocity, Vector2Scale(normal, impulse * physics_b->mass));
	physics_b->velocity =
		Vector2Add(physics_b->velocity, Vector2Scale(normal, impulse * physics_a->mass));

	return impulse * total_mass;
}

void physics_ball_simulation(v3 *out_position,
							 v3 *out_velocity,
							 v3 accel,
							 f32 mass,
							 f32 radius,
							 v3 after_touch_accel,
							 b32 *out_on_the_ground,
							 b32 *out_bounced,
							 struct game_state *game_state)
{
	v3 position = *out_position;
	v3 velocity = *out_velocity;

	/*
     * Drag in the air calculation
     *
     * Formula for drag is FD=1/2CρAv^2
     * A (soccer ball) = 0.038 m^2
     * C (sphere) = 0.45
     * p (air) = 1.21 kg/m3
     *
     * Formula needs meters so we need to convert from pixels to meters and back
     */
	f32 drag_force = 0.5f * game_state->constants.ball_drag_coefficient *
					 game_state->constants.air_density *
					 game_state->constants.ball_cross_sectional_area *
					 powf(Vector3Length(velocity) / game_state->constants.pixels_per_meter, 2) *
					 game_state->constants.pixels_per_meter;
	accel = Vector3Add(accel,
					   Vector3Scale(Vector3Negate(Vector3Normalize(velocity)), drag_force / mass));

	// If ball is in the air apply gravity
	*out_on_the_ground = true;
	if (position.y > radius) {
		*out_on_the_ground = false;
		accel = Vector3Add(accel, game_state->constants.gravity_force);
	} else {
		// Calculate rolling resistance based on velocity and surface properties
		v2 horizontal_velocity = {velocity.x, velocity.z};
		f32 speed = Vector3Length(velocity);

		// Rolling resistance coefficient varies with speed
		f32 rolling_resistance = game_state->constants.ball_rolling_resistance_mult *
								 (1.0f + speed * 0.001f); // Increases slightly with speed

		// Normal force for rolling resistance calculation
		f32 normal_force = mass * 9.81f; // Using gravity constant

		// Rolling resistance force
		f32 resistance_force = rolling_resistance * normal_force;

		// Calculate deceleration from rolling resistance
		f32 deceleration = resistance_force / mass;

		// Apply deceleration in direction opposing motion
		v2 direction = Vector2Scale(Vector2Normalize(horizontal_velocity), -1.0f);
		accel.x += direction.x * deceleration;
		accel.z += direction.y * deceleration;
	}

	accel = Vector3Add(accel, after_touch_accel);

	// Apply acceleration...
	velocity = Vector3Add(velocity, Vector3Scale(accel, game_state->dt));
	if (Vector3Length(velocity) > game_state->constants.ball_max_velocity) {
		velocity =
			Vector3Scale(Vector3Normalize(velocity), game_state->constants.ball_max_velocity);
	}
	// ...and velocity
	position = Vector3Add(position, Vector3Scale(velocity, game_state->dt));

	// Make the ball bounce
	*out_bounced = false;
	if (position.y < radius) {
		*out_bounced = true;
		position.y = radius;
		accel.y *= game_state->constants.ball_cr;
		velocity.y *= -game_state->constants.ball_cr;
		if (fabsf(velocity.y) < 10.0f) {
			velocity.y = 0;
		}
	}

	*out_velocity = velocity;
	*out_position = position;
}
