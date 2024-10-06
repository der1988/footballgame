#include "football.h"
#include <raylib.h>
#include <raymath.h>

struct TeamFormationPosition *football_tactics_get_team_formation(enum TeamFormation formation)
{
    return TeamFormations[formation];
}

struct TeamFormationPosition *
football_tactics_get_team_formation_position(enum TeamFormation formation, u32 position_idx)
{
    return &TeamFormations[formation][position_idx];
}
