#include "types.h"
#include "football.h"
#include "game.h"

bool football_pitch_is_position_inside_penalty_area(v2 pos, enum PitchSide side,
						    struct GameState *game_state)
{
    return (CheckCollisionPointRec(pos, game_state->vars.pitch.goal[side].area));
}

bool football_pitch_is_position_inside(v2 pos, struct GameState *game_state)
{
    return (CheckCollisionPointRec(pos, game_state->vars.pitch.field));
}
