#ifndef SYSTEMS_H
#define SYSTEMS_H

#include "game.h"

void systems_controls(struct GameState *game_state, struct input *input);
void systems_ball(struct GameState *game_state, struct input *input);
void systems_player(struct GameState *game_state, struct input *input);
void systems_goal(struct GameState *game_state);
void systems_goalie(struct GameState *game_state);
void systems_action(struct GameState *game_state);
void systems_camera(struct GameState *game_state);
void systems_render(struct GameState *game_state);
void systems_render_hud(struct GameState *game_state);
void systems_render_debug(struct GameState *game_state);

#endif // !SYSTEMS_H
