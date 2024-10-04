#ifndef FOOTBALL
#define FOOTBALL

#include "types.h"
#include <raylib.h>

// Forward declaration
struct GameState;

typedef enum MatchState {
    MS_KICKOFF,
    MS_PLAY,
    MS_GOAL,
    MS_THROW_IN,
    MS_GOAL_KICK,
    MS_CORNER_KICK,
    MS_PENALTY_KICK,
    MS_FREE_KICK,
    MS_COUNT
} MatchState;

static const char *MatchStateToString[MS_COUNT] = {
    "kickoff", "play", "goal", "throw_in", "goal_kick", "corner_kick", "penalty_kick", "free_kick"};

typedef struct MatchScore {
    u16 home;
    u16 away;
} MatchScore;

typedef struct Match {
    enum MatchState state;
    MatchScore score;
} Match;

typedef enum {
    PT_NORMAL,
    PT_MUD,
    PT_SNOW
} PitchType;

typedef enum {
    PSI_NORTH,
    PSI_SOUTH,
    PSI_COUNT
} PitchSide;

typedef struct {
    Rectangle area;
    v2 center;
} Goal;

typedef struct Pitch {
    Rectangle field;
    Goal goal[PSI_COUNT];
    PitchType type;
} Pitch;

void football_match_change_state(struct GameState *game_state, MatchState new_state);

typedef enum {
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
} PlayerRole;

typedef enum {
    TF_433,
    TF_442,
    TF_352,
    TF_343,
    TF_532,
    TF_4312,
    TF_4231,
    TF_COUNT
} TeamFormation;

typedef enum {
    SOP_BALANCED,
    SOP_OFFENSIVE,
    SOP_DEFENSIVE
} StyleOfPlay;

typedef enum {
    TM_DEFENDING,
    TM_DEFAULT,
    TM_ATTACKING,
    TM_COUNT
} TeamMomentum;

typedef struct {
    const char *name;
    PlayerRole supported_roles[3];
    v2 position[TM_COUNT];
    StyleOfPlay style_of_play;
} TeamFormationPosition;

TeamFormationPosition *football_get_team_formation(TeamFormation formation);

TeamFormationPosition *football_get_team_formation_position(TeamFormation formation,
                                                            u32 position_idx);

#endif // !FOOTBALL
