#ifndef SYSTEMS_H
#define SYSTEMS_H

// Forward declaration
struct game_state;
struct input;

void systems_controls(struct game_state *game_state);
void systems_ball(struct game_state *game_state);
void systems_player_collisions(struct game_state *game_state);
void systems_player_selection(struct game_state *game_state);
void systems_player_control_human(struct game_state *game_state);
void systems_player_control_ai(struct game_state *game_state);
void systems_player_physics(struct game_state *game_state);
void systems_player_on_the_ball(struct game_state *game_state);
void systems_team_pressing(struct game_state *game_state);
void systems_goal(struct game_state *game_state);
void systems_goalie(struct game_state *game_state);
void systems_action(struct game_state *game_state);
void systems_camera(struct game_state *game_state);
void systems_render(struct game_state *game_state);
void systems_render_hud(struct game_state *game_state);
void systems_render_debug(struct game_state *game_state);
void systems_time_machine(struct game_state *game_state);

#endif // !SYSTEMS_H
