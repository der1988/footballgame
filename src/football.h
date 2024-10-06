#ifndef FOOTBALL
#define FOOTBALL

#include "types.h"
#include <raylib.h>

// Forward declaration
struct GameState;

enum MatchState
{
    MS_KICKOFF,
    MS_PLAY,
    MS_GOAL,
    MS_THROW_IN,
    MS_GOAL_KICK,
    MS_CORNER_KICK,
    MS_PENALTY_KICK,
    MS_FREE_KICK,
    MS_COUNT
};

static const char *MatchStateToString[MS_COUNT] = {
    "kickoff", "play", "goal", "throw_in", "goal_kick", "corner_kick", "penalty_kick", "free_kick"};

struct Match
{
    enum MatchState state;
    struct
    {
	u16 home;
	u16 away;
    } score;
};

enum PitchType
{
    PT_NORMAL,
    PT_MUDDY,
    PT_SNOWY
};

enum PitchSide
{
    PSI_NORTH,
    PSI_SOUTH,
    PSI_COUNT
};

struct Goal
{
    Rectangle area;
    v2 center;
};

struct Pitch
{
    Rectangle field;
    struct Goal goal[PSI_COUNT];
    enum PitchType type;
};

void football_match_change_state(struct GameState *game_state, enum MatchState new_state);

enum PlayerRole
{
    PR_GK,  // Goalkeeper
    PR_SB,  // Side Back
    PR_CB,  // Centre Back
    PR_DMF, // Defensive Midfielder
    PR_CMF, // Central Midfielder
    PR_WB,  // Wing Back
    PR_SMF, // Side Midfielder
    PR_AMF, // Attacking Midfielder
    PR_WF,  // Wing Forward
    PR_SS,  // Second Striker
    PR_CF,  // Centre Forward
    PR_COUNT
};

enum TeamFormation
{
    TF_433,
    TF_442,
    TF_352,
    TF_343,
    TF_532,
    TF_4312,
    TF_4231,
    TF_COUNT
};

enum StyleOfPlay
{
    SOP_BALANCED,
    SOP_OFFENSIVE,
    SOP_DEFENSIVE,
    SOP_COUNT
};

enum TeamMomentum
{
    TM_DEFENDING,
    TM_DEFAULT,
    TM_ATTACKING,
    TM_COUNT
};

struct TeamFormationPosition
{
    const char *name;
    enum PlayerRole supported_roles[3];
    v2 position[TM_COUNT];
    enum StyleOfPlay style_of_play;
};

struct TeamFormationPosition *football_get_team_formation(enum TeamFormation formation);

struct TeamFormationPosition *football_get_team_formation_position(enum TeamFormation formation,
								   u32 position_idx);

#endif // !FOOTBALL
