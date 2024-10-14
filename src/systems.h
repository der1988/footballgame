#ifndef SYSTEMS_H
#define SYSTEMS_H

#include "game.h"

void systems_controls(struct game_state *game_state, struct input *input);
void systems_ball(struct game_state *game_state, struct input *input);
void systems_player(struct game_state *game_state, struct input *input);
void systems_player_on_the_ball(struct game_state *game_state, struct input *input);
void systems_player_selection(struct game_state *game_state, struct input *input);
void systems_goal(struct game_state *game_state);
void systems_goalie(struct game_state *game_state);
void systems_action(struct game_state *game_state);
void systems_camera(struct game_state *game_state);
void systems_render(struct game_state *game_state);
void systems_render_hud(struct game_state *game_state);
void systems_render_debug(struct game_state *game_state);

#endif // !SYSTEMS_H
