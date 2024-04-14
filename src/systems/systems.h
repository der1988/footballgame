#include "../game.h"

void systems_ball(GameState *game_state, struct Input *input);
void systems_player(GameState *game_state, struct Input *input);
void systems_goal(GameState *game_state);
void systems_action(GameState *game_state);
void systems_camera(GameState *game_state);
void systems_render(GameState *game_state);
void systems_render_debug(GameState *game_state);
