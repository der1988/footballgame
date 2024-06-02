#ifndef FOOTBALL
#define FOOTBALL

#include "types.h"

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

typedef enum PitchType { PT_NORMAL, PT_MUD, PT_SNOW } PitchType;

typedef struct Pitch {
    Rectangle rectangle;
    PitchType type;
} Pitch;

void football_match_change_state(struct GameState *game_state, MatchState new_state);

#endif // !FOOTBALL
