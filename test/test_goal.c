#include "asset_manager.h"
#include "entity.h"
#include "entity_ball.h"
#include "entity_components.h"
#include "entity_goal.h"
#include "football.h"
#include "game.h"
#include "memory.h"
#include "system.h"
#include "types.h"
#include "unity/unity.h"
#include "unity/unity_internals.h"
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>

static struct game_state *g_game_state = NULL;
static void *g_memory = NULL;
static memory_arena_handle_t *g_main_memory_arena = NULL;

static struct entity *g_ball_entity = NULL;

void setUp(void)
{
	const int width = 800;
	const int height = 600;
	InitWindow(width, height, "Test Goal");

	SetTargetFPS(60);

	g_memory = malloc(Megabytes(128));

	g_main_memory_arena = memory_arena_create_default("main", g_memory, Megabytes(128));

	struct assets assets = asset_manager_load_everything(g_main_memory_arena);

	struct game_config game_config =
		(struct game_config){.screen_width = width, .screen_height = height};

	g_game_state = game_play_init(game_config, assets, g_main_memory_arena);

	// A test build skips gameplay_init (game_play.c): the pitch and the goals are there, the ball
	// and the match are not.
	g_game_state->time = 10;
	g_game_state->gameplay.state_machine.current_state = FS_PLAY;

	g_ball_entity = entity_ball_create(
		(struct ball_config){.position = g_game_state->vars.pitch.midfield_spot}, g_game_state);
	g_game_state->vars.ball.entity = g_ball_entity;
}

void tearDown(void)
{
	game_play_destroy(g_game_state);

	memory_arena_destroy(g_main_memory_arena);

	free(g_memory);

	CloseWindow();
}

// One physics step that took the ball from 'from' to 'to'
static void ball_steps(v3 from, v3 to, v3 velocity)
{
	entity_ball_set_position_and_velocity(g_ball_entity, to, velocity);
	g_ball_entity->ball.previous_position = from;
}

static struct goal_collision impact(enum pitch_side side)
{
	return entity_goal_resolve_ball_impact(
		g_game_state->vars.pitch.goal[side].entity, g_ball_entity, g_game_state);
}

// A north goal shot along z, crossing the line at (x, height)
static void north_shot_crossing_at(f32 x, f32 height)
{
	const f32 line = g_game_state->vars.pitch.goal[PSI_NORTH].center.y;
	ball_steps((v3){.x = x, .y = height, .z = line + 1},
			   (v3){.x = x, .y = height, .z = line - 1},
			   (v3){.z = -480});
}

static rect_t north_goal_rectangle(void)
{
	return entity_goal_rectangle(g_game_state->vars.pitch.goal[PSI_NORTH].entity);
}

static f32 ball_radius(void)
{
	return g_ball_entity->collider.colliders[0].props.circle.radius;
}

void test_a_shot_through_the_mouth_scores_where_it_crosses_the_line(void)
{
	const struct goal *goal = &g_game_state->vars.pitch.goal[PSI_NORTH];
	// Three quarters of the way through the step
	ball_steps((v3){.x = goal->center.x - 4, .y = 12, .z = goal->center.y + 3},
			   (v3){.x = goal->center.x + 4, .y = 8, .z = goal->center.y - 1},
			   (v3){.x = 960, .y = -480, .z = -480});

	struct goal_collision collision = impact(PSI_NORTH);

	TEST_ASSERT_TRUE(collision.hit);
	TEST_ASSERT_EQUAL_INT(GW_NONE, collision.woodwork);
	TEST_ASSERT_TRUE(collision.scored);
	TEST_ASSERT_EQUAL_FLOAT(goal->center.x + 2, collision.point.x);
	TEST_ASSERT_EQUAL_FLOAT(9, collision.point.y);
}

void test_a_shot_into_the_south_goal_scores(void)
{
	const struct goal *goal = &g_game_state->vars.pitch.goal[PSI_SOUTH];
	ball_steps((v3){.x = goal->center.x, .y = 10, .z = goal->center.y - 1},
			   (v3){.x = goal->center.x, .y = 10, .z = goal->center.y + 1},
			   (v3){.z = 480});

	struct goal_collision collision = impact(PSI_SOUTH);

	TEST_ASSERT_TRUE(collision.hit);
	TEST_ASSERT_TRUE(collision.scored);
}

void test_a_shot_over_the_bar_misses_the_goal(void)
{
	north_shot_crossing_at(g_game_state->vars.pitch.goal[PSI_NORTH].center.x,
						   north_goal_rectangle().height + ball_radius() + 1);

	TEST_ASSERT_FALSE(impact(PSI_NORTH).hit);
}

void test_a_shot_wide_of_the_post_misses_the_goal(void)
{
	north_shot_crossing_at(north_goal_rectangle().x - ball_radius() - 1, 10);

	TEST_ASSERT_FALSE(impact(PSI_NORTH).hit);
}

void test_a_shot_onto_the_left_post_hits_it(void)
{
	north_shot_crossing_at(north_goal_rectangle().x + 1, 10);

	struct goal_collision collision = impact(PSI_NORTH);

	TEST_ASSERT_TRUE(collision.hit);
	TEST_ASSERT_EQUAL_INT(GW_LEFT_POST, collision.woodwork);
}

void test_a_shot_onto_the_right_post_hits_it(void)
{
	const rect_t goal_rectangle = north_goal_rectangle();
	north_shot_crossing_at(goal_rectangle.x + goal_rectangle.width - 1, 10);

	struct goal_collision collision = impact(PSI_NORTH);

	TEST_ASSERT_TRUE(collision.hit);
	TEST_ASSERT_EQUAL_INT(GW_RIGHT_POST, collision.woodwork);
}

void test_a_shot_onto_the_crossbar_hits_it(void)
{
	north_shot_crossing_at(g_game_state->vars.pitch.goal[PSI_NORTH].center.x,
						   north_goal_rectangle().height - 1);

	struct goal_collision collision = impact(PSI_NORTH);

	TEST_ASSERT_TRUE(collision.hit);
	TEST_ASSERT_EQUAL_INT(GW_CROSSBAR, collision.woodwork);
}

// The centre of the ball passes over the bar, its lower half does not.
void test_a_shot_clipping_the_top_of_the_crossbar_hits_it(void)
{
	north_shot_crossing_at(g_game_state->vars.pitch.goal[PSI_NORTH].center.x,
						   north_goal_rectangle().height + ball_radius() * 0.5f);

	struct goal_collision collision = impact(PSI_NORTH);

	TEST_ASSERT_TRUE(collision.hit);
	TEST_ASSERT_EQUAL_INT(GW_CROSSBAR, collision.woodwork);
}

// Struck at this height, the bar sends the ball almost straight down, with a z step too small to
// move it off the line in float.
void test_a_ball_left_on_the_line_by_the_crossbar_is_deflected_once(void)
{
	const f32 line = g_game_state->vars.pitch.goal[PSI_NORTH].center.y;
	north_shot_crossing_at(g_game_state->vars.pitch.goal[PSI_NORTH].center.x, 27.4999981f);

	u32 deflections = 0;
	v3 velocity = g_ball_entity->physics3d.velocity;
	system_goal(g_game_state);
	TEST_ASSERT_EQUAL_FLOAT(line, g_ball_entity->transform.position.y);

	// Until it lands, when the bounce would count as a deflection
	for (u32 i = 0; i < 120 && g_ball_entity->transform.virtual_height > ball_radius(); ++i) {
		v3 new_velocity = g_ball_entity->physics3d.velocity;
		if (Vector3Distance(velocity, new_velocity) > 20) {
			++deflections;
		}
		velocity = new_velocity;

		g_game_state->time += g_game_state->dt;
		system_ball(g_game_state);
		system_goal(g_game_state);
	}

	TEST_ASSERT_EQUAL_UINT32(1, deflections);
}

void test_a_ball_stopping_on_the_line_scores_on_the_step_that_takes_it_over(void)
{
	const struct goal *goal = &g_game_state->vars.pitch.goal[PSI_NORTH];
	const v3 on_the_line = {.x = goal->center.x, .y = 10, .z = goal->center.y};

	ball_steps((v3){.x = goal->center.x, .y = 10, .z = goal->center.y + 1},
			   on_the_line,
			   (v3){.z = -480});
	TEST_ASSERT_FALSE(impact(PSI_NORTH).hit);

	ball_steps(on_the_line,
			   (v3){.x = goal->center.x, .y = 10, .z = goal->center.y - 1},
			   (v3){.z = -480});
	struct goal_collision collision = impact(PSI_NORTH);
	TEST_ASSERT_TRUE(collision.hit);
	TEST_ASSERT_TRUE(collision.scored);
}

void test_a_ball_leaving_the_line_back_into_the_field_does_not_score(void)
{
	const struct goal *goal = &g_game_state->vars.pitch.goal[PSI_NORTH];
	ball_steps((v3){.x = goal->center.x, .y = 10, .z = goal->center.y},
			   (v3){.x = goal->center.x, .y = 10, .z = goal->center.y + 1},
			   (v3){.z = 480});

	TEST_ASSERT_FALSE(impact(PSI_NORTH).hit);
}

void test_a_ball_already_behind_the_line_does_not_score(void)
{
	const struct goal *goal = &g_game_state->vars.pitch.goal[PSI_NORTH];
	ball_steps((v3){.x = goal->center.x, .y = 10, .z = goal->center.y - 3},
			   (v3){.x = goal->center.x, .y = 10, .z = goal->center.y - 4},
			   (v3){.z = -480});

	TEST_ASSERT_FALSE(impact(PSI_NORTH).hit);
}

// A ball coming down onto the top half of the crossbar is sent up and on over the goal. It must
// not come back to the bar for a second deflection.
void test_a_ball_sent_over_the_goal_by_the_crossbar_is_deflected_once(void)
{
	const struct goal *goal = &g_game_state->vars.pitch.goal[PSI_NORTH];
	const f32 line = goal->center.y;

	entity_ball_set_position_and_velocity(g_ball_entity,
										  (v3){.x = goal->center.x, .y = 36, .z = line + 5},
										  (v3){.y = -150, .z = -150});

	u32 deflections = 0;
	v3 velocity = g_ball_entity->physics3d.velocity;
	for (u32 i = 0; i < 120; ++i) {
		g_game_state->time += g_game_state->dt;
		system_ball(g_game_state);
		system_goal(g_game_state);

		v3 new_velocity = g_ball_entity->physics3d.velocity;
		if (Vector3Distance(velocity, new_velocity) > 20) {
			++deflections;
		}
		velocity = new_velocity;
	}

	TEST_ASSERT_EQUAL_UINT32(1, deflections);
	TEST_ASSERT_FALSE(goal->entity->goal.scored);
	TEST_ASSERT_TRUE(g_ball_entity->transform.position.y < line);
	TEST_ASSERT_TRUE(g_ball_entity->physics3d.velocity.y > 0);
	TEST_ASSERT_TRUE(g_ball_entity->physics3d.velocity.z < 0);
}

int main(void)
{
	UNITY_BEGIN();
	RUN_TEST(test_a_shot_through_the_mouth_scores_where_it_crosses_the_line);
	RUN_TEST(test_a_shot_into_the_south_goal_scores);
	RUN_TEST(test_a_shot_over_the_bar_misses_the_goal);
	RUN_TEST(test_a_shot_wide_of_the_post_misses_the_goal);
	RUN_TEST(test_a_shot_onto_the_left_post_hits_it);
	RUN_TEST(test_a_shot_onto_the_right_post_hits_it);
	RUN_TEST(test_a_shot_onto_the_crossbar_hits_it);
	RUN_TEST(test_a_shot_clipping_the_top_of_the_crossbar_hits_it);
	RUN_TEST(test_a_ball_left_on_the_line_by_the_crossbar_is_deflected_once);
	RUN_TEST(test_a_ball_stopping_on_the_line_scores_on_the_step_that_takes_it_over);
	RUN_TEST(test_a_ball_leaving_the_line_back_into_the_field_does_not_score);
	RUN_TEST(test_a_ball_already_behind_the_line_does_not_score);
	RUN_TEST(test_a_ball_sent_over_the_goal_by_the_crossbar_is_deflected_once);
	return UNITY_END();
}
