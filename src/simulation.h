#include "types.h"

// Forward declaration
struct game_state;

void sim_ball_physics(v3 *out_position,
		      v3 *out_velocity,
		      v3 accel,
		      f32 mass,
		      f32 radius,
		      b32 *out_on_the_ground,
		      b32 *out_bounced,
		      struct game_state *game_state);
