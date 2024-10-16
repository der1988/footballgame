#include "types.h"
#include "football.h"
#include "game.h"

b32 football_pitch_is_position_inside_penalty_area(v2 pos,
						   enum pitch_side side,
						   struct game_state *game_state)
{
    return (CheckCollisionPointRec(pos, game_state->vars.pitch.goal[side].area));
}

b32 football_pitch_is_position_inside(v2 pos, struct game_state *game_state)
{
    return (CheckCollisionPointRec(pos, game_state->vars.pitch.field));
}
