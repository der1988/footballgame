#include "football/football.h"
#include "entities/e_goal.h"
#include "game/game.h"
#include <raylib.h>

static struct goal setup_goal(struct entity *entity,
							  enum pitch_side side,
							  v2 center,
							  u16 width,
							  u16 height,
							  u16 penalty_box_width,
							  u16 penalty_box_height);

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

void football_pitch_setup(struct game_state *game_state)
{

	game_state->vars.pitch.field = (rect_t){.x = 72, .y = 233, .width = 856, .height = 360 * 3 - 2};
	game_state->vars.pitch.goal_width = 100;
	game_state->vars.pitch.goal_height = 32;
	game_state->vars.pitch.penalty_box_width = 478;
	game_state->vars.pitch.penalty_box_height = 149;
	game_state->vars.pitch.midfield_spot =
		(v2){.x = game_state->vars.pitch.field.x + game_state->vars.pitch.field.width * 0.5f,
			 .y = game_state->vars.pitch.field.y + game_state->vars.pitch.field.height * 0.5f};

	// Set up goals
	for (usize i = 0; i < PSI_COUNT; ++i) {

		v2 goal_center =
			(v2){.x = game_state->vars.pitch.field.x + game_state->vars.pitch.field.width * 0.5f,
				 .y = game_state->vars.pitch.field.y + game_state->vars.pitch.field.height * i};

		struct entity *goal_entity =
			entity_goal_create((struct goal_config){.side = i, .center = goal_center}, game_state);

		game_state->vars.pitch.goal[i] = setup_goal(goal_entity,
													i,
													goal_center,
													game_state->vars.pitch.goal_width,
													game_state->vars.pitch.goal_height,
													game_state->vars.pitch.penalty_box_width,
													game_state->vars.pitch.penalty_box_height);
	}
}

static struct goal setup_goal(struct entity *entity,
							  enum pitch_side side,
							  v2 center,
							  u16 width,
							  u16 height,
							  u16 penalty_box_width,
							  u16 penalty_box_height)
{
	struct goal goal = (struct goal){
		.entity = entity,
		.center = center,
		.penalty_box = (rect_t){.x = center.x - penalty_box_width * 0.5f,
								.y = center.y - (side == PSI_NORTH ? 0 : penalty_box_height),
								.width = penalty_box_width,
								.height = penalty_box_height},
		.penalty_spot = {.x = center.x,
						 .y = center.y + 11 * PIXELS_PER_METER * (side == PSI_NORTH ? 1 : -1)},
		.goal_quad_points = {
			{.x = center.x - width * 2, .y = 0, .z = center.y},
			{.x = center.x - width * 2, .y = height, .z = center.y},
			{.x = center.x + width * 2, .y = height, .z = center.y},
			{.x = center.x + width * 2, .y = 0, .z = center.y},
		}};

	return goal;
}
