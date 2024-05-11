#include "../game.h"
#include "../types.h"

void sim_ball_physics(v3 *out_position, v3 *out_velocity, v3 accel, f32 mass,
                      b32 *out_on_the_ground, b32 *out_bounced, GameState *game_state);
