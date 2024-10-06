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

static struct TeamFormationPosition TeamFormations[TF_COUNT][10] = {
    // TF_433
    {{.name = "LB",
      .style_of_play = SOP_BALANCED,
      .supported_roles = {PR_SB},
      .position = {{.x = -0.6f, .y = 0.2f}, {.x = -0.8f, .y = 0.3f}, {.x = -0.7f, .y = 0.6f}}},
     {.name = "LB",
      .style_of_play = SOP_BALANCED,
      .supported_roles = {PR_SB},
      .position = {{.x = 0.6f, .y = 0.2f}, {.x = 0.8f, .y = 0.3f}, {.x = 0.7f, .y = 0.6f}}},
     {.name = "CB",
      .style_of_play = SOP_DEFENSIVE,
      .supported_roles = {PR_CB},
      .position = {{.x = -0.2f, .y = 0.1f}, {.x = -0.2f, .y = 0.2f}, {.x = -0.2f, .y = 0.5f}}},
     {.name = "CB",
      .style_of_play = SOP_DEFENSIVE,
      .supported_roles = {PR_CB},
      .position = {{.x = 0.2f, .y = 0.1f}, {.x = 0.2f, .y = 0.2f}, {.x = 0.2f, .y = 0.5f}}},
     {.name = "DMF",
      .style_of_play = SOP_BALANCED,
      .supported_roles = {PR_DMF},
      .position = {{.x = 0, .y = 0.2f}, {.x = 0, .y = 0.4f}, {.x = 0, .y = 0.7f}}},
     {.name = "CMF",
      .style_of_play = SOP_BALANCED,
      .supported_roles = {PR_CMF},
      .position = {{.x = -0.3, .y = 0.3f}, {.x = -0.3f, .y = 0.5f}, {.x = -0.3f, .y = 0.8f}}},
     {.name = "CMF",
      .style_of_play = SOP_BALANCED,
      .supported_roles = {PR_CMF},
      .position = {{.x = 0.3, .y = 0.3f}, {.x = 0.3f, .y = 0.5f}, {.x = 0.3f, .y = 0.8f}}},
     {.name = "LWF",
      .style_of_play = SOP_OFFENSIVE,
      .supported_roles = {PR_WF},
      .position = {{.x = -0.7f, .y = 0.35f}, {.x = -0.7f, .y = 0.6f}, {.x = -0.7f, .y = 0.9f}}},
     {.name = "RWF",
      .style_of_play = SOP_OFFENSIVE,
      .supported_roles = {PR_WF},
      .position = {{.x = 0.7f, .y = 0.35f}, {.x = 0.7f, .y = 0.6f}, {.x = 0.7f, .y = 0.9f}}},
     {.name = "CF",
      .style_of_play = SOP_BALANCED,
      .supported_roles = {PR_CF},
      .position = {{.x = 0, .y = 0.5f}, {.x = 0, .y = 0.7f}, {.x = 0, .y = 0.9f}}}}};

struct TeamFormationPosition *football_tactics_get_team_formation(enum TeamFormation formation);

struct TeamFormationPosition *
football_tactics_get_team_formation_position(enum TeamFormation formation, u32 position_idx);

bool football_pitch_is_position_inside_penalty_area(v2 pos, enum PitchSide side,
						    struct GameState *game_state);
bool football_pitch_is_position_inside(v2 pos, struct GameState *game_state);

#endif // !FOOTBALL
