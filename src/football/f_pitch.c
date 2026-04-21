#include "football.h"
#include "game.h"
#include <raylib.h>

b32 football_pitch_is_position_inside_penalty_area(v2 pos,
												   enum pitch_side side,
												   struct game_state *game_state)
{
	return (CheckCollisionPointRec(pos, game_state->vars.pitch.goal[side].penalty_box));
}

b32 football_pitch_is_position_inside(v2 pos, struct game_state *game_state)
{
	return (CheckCollisionPointRec(pos, game_state->vars.pitch.field));
}

b32 football_pitch_is_position_inside_cross_area(v2 pos,
												 enum cross_area *cross_area,
												 struct game_state *game_state)
{
	b32 is_left = pos.x < game_state->vars.pitch.field.x + game_state->vars.pitch.cross_area_width;
	b32 is_right = pos.x > game_state->vars.pitch.field.x + game_state->vars.pitch.goal_width -
							   game_state->vars.pitch.cross_area_width;
	b32 is_up = pos.y < game_state->vars.pitch.field.y + game_state->vars.pitch.cross_area_height;
	b32 is_down = pos.y > game_state->vars.pitch.field.y + game_state->vars.pitch.field.height -
							  game_state->vars.pitch.cross_area_height;

	b32 is_inside = false;
	if (is_left) {
		if (is_down) {
			*cross_area = CROSS_AREA_BOTTOM_LEFT;
			is_inside = true;
		} else if (is_up) {
			*cross_area = CROSS_AREA_UPPER_LEFT;
			is_inside = true;
		}
	} else if (is_right) {
		if (is_down) {
			*cross_area = CROSS_AREA_BOTTOM_RIGHT;
			is_inside = true;
		} else if (is_up) {
			*cross_area = CROSS_AREA_UPPER_RIGHT;
			is_inside = true;
		}
	}
	return is_inside;
}
