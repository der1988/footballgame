#include "components/components.h"
#include "game/game.h"
#include "types.h"
#include <raymath.h>

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
