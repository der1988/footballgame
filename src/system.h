#ifndef SYSTEM_H
#define SYSTEM_H

// Forward declaration
struct game_state;
struct input;

void system_ball(struct game_state *game_state);
void system_player_collisions(struct game_state *game_state);
void system_player_selection(struct game_state *game_state);
void system_player_control_human(struct game_state *game_state);
void system_player_control_ai(struct game_state *game_state);
void system_player_physics(struct game_state *game_state);
void system_players_off_the_ball(struct game_state *game_state);
void system_player_on_the_ball(struct game_state *game_state);
void system_team_pressing(struct game_state *game_state);
void system_goal(struct game_state *game_state);
void system_goalie(struct game_state *game_state);
void system_action(struct game_state *game_state);
void system_camera(struct game_state *game_state);
void system_referee(struct game_state *game_state);
void system_render(struct game_state *game_state);
void system_render_hud_front(struct game_state *game_state);
void system_render_hud_back(struct game_state *game_state);
void system_render_debug(struct game_state *game_state);
void system_time_machine(struct game_state *game_state);
void system_oob_checks(struct game_state *game_state);
#endif // !SYSTEM_H
