#include "football.h"
#include <raylib.h>
#include <raymath.h>

struct team_formation_position *football_tactics_get_team_formation(enum team_formation formation)
{
    return TeamFormations[formation];
}

struct team_formation_position *
football_tactics_get_team_formation_position(enum team_formation formation, u32 position_idx)
{
    return &TeamFormations[formation][position_idx];
}
