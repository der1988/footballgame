#include "../game.h"
#include "football/football.h"
#include "utils.h"
#include <raylib.h>

void football_match_change_state(struct GameState *game_state, MatchState new_state) {
    game_state->vars.match.state = new_state;
    GLOGF_INFO(LS_GAME, "match state changed to: %s", MatchStateToString[new_state]);
}
