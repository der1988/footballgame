#include "ai.h"
#include "constants.h"
#include "asset_manager.h"
#include "entity.h"
#include "entity_ball.h"
#include "entity_components.h"
#include "entity_player.h"
#include "football.h"
#include "game.h"
#include "log.h"
#include "math_ext.h"
#include "memory.h"
#include "system.h"
#include "types.h"
#include "unity/unity.h"
#include "unity/unity_internals.h"
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>

#define WIDE_ANGLE_DEG 45.0f

// Who a pass goes to. The control systems pick the team mate once and put him in the controls;
// system_player_on_the_ball plays the ball to that man and never picks again.

static struct game_state *g_game_state = NULL;
static void *g_memory = NULL;
static memory_arena_handle_t *g_main_memory_arena = NULL;

static struct entity *g_ball_entity = NULL;
static struct entity *g_passer_entity = NULL;
static struct entity *g_team_mate_entity = NULL;
static struct entity *g_other_team_mate_entity = NULL;
static struct entity *g_opponent_entity = NULL;

void setUp(void)
{
	const int width = 800;
	const int height = 600;
	InitWindow(width, height, "Test Passing");

	SetTargetFPS(60);

	g_memory = malloc(Megabytes(128));

	g_main_memory_arena = memory_arena_create_default("main", g_memory, Megabytes(128));

	struct assets assets = asset_manager_load_everything(g_main_memory_arena);

	struct game_config game_config =
		(struct game_config){.screen_width = width, .screen_height = height};

	g_game_state = game_play_init(game_config, assets, g_main_memory_arena);

	// A test build skips gameplay_init (game_play.c), so there is no match behind this: no ball, no
	// players, no sides and a clock at zero. The bits these rules read are set here. The home side
	// attacks towards +y.
	g_game_state->time = 10;
	g_game_state->gameplay.state_machine.current_state = FS_PLAY;
	g_game_state->vars.teams[TS_HOME].play_context.pitch_side = PSI_NORTH;
	g_game_state->vars.teams[TS_AWAY].play_context.pitch_side = PSI_SOUTH;

	const v2 midfield_spot = g_game_state->vars.pitch.midfield_spot;

	g_ball_entity =
		entity_ball_create((struct ball_config){.position = midfield_spot}, g_game_state);
	g_game_state->vars.ball.entity = g_ball_entity;

	g_passer_entity = entity_player_create(
		(struct player_config){.position = midfield_spot, .team_side = TS_HOME}, g_game_state);
	g_team_mate_entity =
		entity_player_create((struct player_config){.team_side = TS_HOME}, g_game_state);
	g_other_team_mate_entity =
		entity_player_create((struct player_config){.team_side = TS_HOME}, g_game_state);
	g_opponent_entity =
		entity_player_create((struct player_config){.team_side = TS_AWAY}, g_game_state);

	// Everybody starts well out of the way; each test places the men it needs
	const v2 far_away = {.x = midfield_spot.x + 2000, .y = midfield_spot.y + 2000};
	g_team_mate_entity->transform.position = far_away;
	g_other_team_mate_entity->transform.position = far_away;
	g_opponent_entity->transform.position = (v2){.x = far_away.x, .y = -far_away.y};
}

void tearDown(void)
{
	game_play_destroy(g_game_state);

	memory_arena_destroy(g_main_memory_arena);

	free(g_memory);

	CloseWindow();
}

static void place_relative_to_passer(struct entity *player_entity, f32 distance, f32 angle_deg)
{
	player_entity->transform.position =
		Vector2Add(g_passer_entity->transform.position,
				   Vector2Rotate((v2){.y = distance}, angle_deg * DEG2RAD));
}

static const v2 UP_THE_PITCH = {.y = 1};

// The searches score distance but never reject on it, so a team mate parked far away still counts.
// Tests about who must never be picked take the team mates off.
static void remove_team_mates(void)
{
	entity_destroy(g_game_state->entity_repository, g_team_mate_entity->header.id);
	entity_destroy(g_game_state->entity_repository, g_other_team_mate_entity->header.id);
}

static f32 pass_range(void) { return g_game_state->constants.ball_pass_auto_max_distance; }

static struct entity *human_search(v2 scan_direction, f32 angle, f32 target_distance)
{
	struct entity *target = NULL;
	entity_player_find_pass_target(
		g_passer_entity, scan_direction, angle, target_distance, pass_range(), &target, g_game_state);
	return target;
}

static struct entity *ai_search(v2 scan_direction, f32 angle, f32 target_distance)
{
	struct entity *target = NULL;
	ai_pass_find_target(g_passer_entity,
						scan_direction,
						angle,
						target_distance,
						pass_range(),
						false,
						&target,
						g_game_state);
	return target;
}

// Where the ball was sent, on the pitch plane.
static v2 ball_direction(void)
{
	return Vector2Normalize(V3_TO_V2(g_ball_entity->physics3d.accel));
}

static v2 direction_to(struct entity *player_entity)
{
	return Vector2Normalize(
		Vector2Subtract(player_entity->transform.position, g_passer_entity->transform.position));
}

// The passer is on the ball on this tick, which is what lets system_player_on_the_ball play it.
static void passer_on_the_ball(void)
{
	g_ball_entity->ball.last_touch_entity_id = g_passer_entity->header.id;
	g_ball_entity->ball.last_touch_time = g_game_state->time;
}

// The searches

// Asking twice must give the same man: a search that marks whoever it picks penalises him the
// second time, so the decision and the pass would disagree.
void test_the_ai_search_gives_the_same_man_when_asked_twice(void)
{
	place_relative_to_passer(g_team_mate_entity, 150, 0);
	place_relative_to_passer(g_other_team_mate_entity, 150, 20);

	struct entity *first = ai_search(UP_THE_PITCH, 90, 150);
	struct entity *second = ai_search(UP_THE_PITCH, 90, 150);

	TEST_ASSERT_NOT_NULL(first);
	TEST_ASSERT_EQUAL_PTR(first, second);
}

void test_the_human_search_gives_the_same_man_when_asked_twice(void)
{
	place_relative_to_passer(g_team_mate_entity, 150, 0);
	place_relative_to_passer(g_other_team_mate_entity, 150, 20);

	struct entity *first = human_search(UP_THE_PITCH, 90, 150);
	struct entity *second = human_search(UP_THE_PITCH, 90, 150);

	TEST_ASSERT_NOT_NULL(first);
	TEST_ASSERT_EQUAL_PTR(first, second);
}

void test_the_searches_never_pick_an_opponent(void)
{
	remove_team_mates();
	place_relative_to_passer(g_opponent_entity, 150, 0);

	TEST_ASSERT_NULL(ai_search(UP_THE_PITCH, 90, 150));
	TEST_ASSERT_NULL(human_search(UP_THE_PITCH, 90, 150));
}

void test_the_searches_never_pick_a_man_outside_the_cone(void)
{
	place_relative_to_passer(g_team_mate_entity, 150, 90);

	TEST_ASSERT_NULL(ai_search(UP_THE_PITCH, 30, 150));
	TEST_ASSERT_NULL(human_search(UP_THE_PITCH, 30, 150));
}

void test_the_searches_never_pick_the_passer(void)
{
	remove_team_mates();

	TEST_ASSERT_NULL(ai_search(UP_THE_PITCH, 180, 0));
	TEST_ASSERT_NULL(human_search(UP_THE_PITCH, 180, 0));
}

// The two searches are kept apart on purpose. A human aims with the stick and the charge, so his
// search takes the man at the charged distance wherever he stands; the AI prefers the man further
// up the pitch.
void test_the_human_search_takes_the_man_at_the_charged_distance_even_behind(void)
{
	place_relative_to_passer(g_team_mate_entity, 100, 170);
	place_relative_to_passer(g_other_team_mate_entity, 250, 0);

	TEST_ASSERT_EQUAL_PTR(g_team_mate_entity, human_search(UP_THE_PITCH, 180, 100));
}

void test_the_ai_search_prefers_the_man_up_the_pitch(void)
{
	place_relative_to_passer(g_team_mate_entity, 100, 170);
	place_relative_to_passer(g_other_team_mate_entity, 250, 0);

	TEST_ASSERT_EQUAL_PTR(g_other_team_mate_entity, ai_search(UP_THE_PITCH, 180, 100));
}

// A man the AI cannot reach must not outrank one it can, or nobody gets the ball
void test_the_ai_search_never_picks_a_man_out_of_range(void)
{
	place_relative_to_passer(g_team_mate_entity, pass_range() + 10, 0);

	TEST_ASSERT_NULL(ai_search(UP_THE_PITCH, 90, pass_range()));
}

void test_the_ai_search_skips_a_screened_man_only_when_asked_to(void)
{
	place_relative_to_passer(g_team_mate_entity, 150, 0);
	place_relative_to_passer(g_opponent_entity, 75, 0);

	TEST_ASSERT_EQUAL_PTR(g_team_mate_entity, ai_search(UP_THE_PITCH, 90, 150));

	struct entity *target = NULL;
	TEST_ASSERT_FALSE(ai_pass_find_target(
		g_passer_entity, UP_THE_PITCH, 90, 150, pass_range(), true, &target, g_game_state));
}

static void team_mate_just_played_the_ball(void)
{
	g_ball_entity->ball.last_play_entity_id = g_team_mate_entity->header.id;
	g_ball_entity->ball.last_play_time = g_game_state->time;
}

void test_the_ai_search_does_not_give_the_ball_straight_back(void)
{
	place_relative_to_passer(g_team_mate_entity, 100, 0);
	place_relative_to_passer(g_other_team_mate_entity, 200, 60);
	team_mate_just_played_the_ball();

	TEST_ASSERT_EQUAL_PTR(g_other_team_mate_entity, ai_search(UP_THE_PITCH, 90, 100));
}

void test_the_ai_search_gives_the_ball_back_once_the_pass_back_time_is_over(void)
{
	place_relative_to_passer(g_team_mate_entity, 100, 0);
	place_relative_to_passer(g_other_team_mate_entity, 200, 60);
	team_mate_just_played_the_ball();
	g_game_state->time += AI_PASS_BACK_TIME;

	TEST_ASSERT_EQUAL_PTR(g_team_mate_entity, ai_search(UP_THE_PITCH, 90, 100));
}

// From the controls to the ball

// The man straight along the steering is the one a search aimed that way would find, so the ball
// going wide proves nobody picked again.
void test_the_pass_goes_to_the_man_in_the_controls(void)
{
	place_relative_to_passer(g_team_mate_entity, 150, WIDE_ANGLE_DEG);
	place_relative_to_passer(g_other_team_mate_entity, 150, 0);
	passer_on_the_ball();
	g_passer_entity->player_controls.steering = UP_THE_PITCH;
	g_passer_entity->player_controls.pass = 0.5f;
	g_passer_entity->player_controls.pass_target_entity_id = g_team_mate_entity->header.id;

	system_player_on_the_ball(g_game_state);

	TEST_ASSERT_EQUAL_UINT32(g_team_mate_entity->header.id,
							 g_ball_entity->ball.pass_target_entity_id);
	TEST_ASSERT_FLOAT_WITHIN(
		0.001f, 1.0f, Vector2DotProduct(ball_direction(), direction_to(g_team_mate_entity)));
}

// The aim point predicts the ball's travel time for a pass along the ground. Lifting it off the
// ground makes the pass too strong and lands it somewhere else.
void test_an_automatic_pass_is_struck_along_the_ground(void)
{
	place_relative_to_passer(g_team_mate_entity, 150, WIDE_ANGLE_DEG);
	passer_on_the_ball();
	g_passer_entity->player_controls.steering = UP_THE_PITCH;
	g_passer_entity->player_controls.pass = 0.5f;
	g_passer_entity->player_controls.pass_target_entity_id = g_team_mate_entity->header.id;

	system_player_on_the_ball(g_game_state);

	const v3 accel = g_ball_entity->physics3d.accel;
	TEST_ASSERT_LESS_THAN_FLOAT(0.01f, fabsf(accel.y) / Vector3Length(accel));
}

void test_a_pass_with_nobody_chosen_goes_along_the_steering_to_nobody(void)
{
	place_relative_to_passer(g_team_mate_entity, 150, WIDE_ANGLE_DEG);
	passer_on_the_ball();
	g_passer_entity->player_controls.steering = UP_THE_PITCH;
	g_passer_entity->player_controls.pass = 0.5f;
	g_passer_entity->player_controls.pass_target_entity_id = EID_NOT_VALID;

	system_player_on_the_ball(g_game_state);

	TEST_ASSERT_EQUAL_UINT32(EID_NOT_VALID, g_ball_entity->ball.pass_target_entity_id);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, Vector2DotProduct(ball_direction(), UP_THE_PITCH));
}

// Where an automatic pass aims

static v2 aim_at_team_mate(void)
{
	const struct player_pass pass = entity_player_aim_pass(g_passer_entity,
														   g_ball_entity,
														   g_team_mate_entity,
														   g_game_state->constants.ball_pass_height,
														   g_game_state);
	return Vector2Add(g_passer_entity->transform.position,
					  Vector2Scale(Vector2Normalize(V3_TO_V2(pass.direction)),
								   pass.power * pass_range()));
}

static void assert_inside_the_margin(v2 point)
{
	const rect_t field = g_game_state->vars.pitch.field;
	TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(field.x + PASS_AIM_FIELD_MARGIN, point.x);
	TEST_ASSERT_LESS_OR_EQUAL_FLOAT(field.x + field.width - PASS_AIM_FIELD_MARGIN, point.x);
	TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(field.y + PASS_AIM_FIELD_MARGIN, point.y);
	TEST_ASSERT_LESS_OR_EQUAL_FLOAT(field.y + field.height - PASS_AIM_FIELD_MARGIN, point.y);
}

// A man running at the touchline is led along his run, only not as far as the line.
void test_a_man_running_at_the_touchline_is_led_short_of_it(void)
{
	const rect_t field = g_game_state->vars.pitch.field;
	const v2 touchline_run = {.x = 1, .y = 1};
	g_passer_entity->transform.position.x = field.x + field.width - 150;
	g_team_mate_entity->transform.position =
		(v2){.x = field.x + field.width - 30, .y = g_passer_entity->transform.position.y + 100};
	g_team_mate_entity->physics.velocity = Vector2Scale(touchline_run, 100);

	const v2 aim_point = aim_at_team_mate();

	assert_inside_the_margin(aim_point);
	const v2 aim_lead = Vector2Subtract(aim_point, g_team_mate_entity->transform.position);
	TEST_ASSERT_GREATER_THAN_FLOAT(0, Vector2Length(aim_lead));
	TEST_ASSERT_FLOAT_WITHIN(0.001f,
							 1.0f,
							 Vector2DotProduct(Vector2Normalize(aim_lead),
											   Vector2Normalize(touchline_run)));
}

// A back pass to the keeper stood on his goal line, walking back towards it: aimed at the line, the
// ball rolls on past him for a corner.
void test_a_pass_to_a_man_on_the_goal_line_aims_inside_the_field(void)
{
	const rect_t field = g_game_state->vars.pitch.field;
	g_passer_entity->transform.position =
		(v2){.x = field.x + field.width * 0.5f, .y = field.y + 200};
	g_team_mate_entity->transform.position =
		(v2){.x = field.x + field.width * 0.5f, .y = field.y + 5};
	g_team_mate_entity->physics.velocity = (v2){.y = -20};

	assert_inside_the_margin(aim_at_team_mate());
}

void test_a_man_already_outside_the_margin_is_aimed_at_from_inside_it(void)
{
	const rect_t field = g_game_state->vars.pitch.field;
	g_team_mate_entity->transform.position =
		(v2){.x = field.x + 2, .y = field.y + field.height * 0.5f};
	g_team_mate_entity->physics.velocity = Vector2Zero();
	g_passer_entity->transform.position =
		(v2){.x = field.x + 150, .y = field.y + field.height * 0.5f};

	assert_inside_the_margin(aim_at_team_mate());
}

// Zeroing the controls leaves entity 0 in the target, and entity 0 is a real entity. The control
// systems must put EID_NOT_VALID back every tick, whatever the man is doing.
void test_the_controls_reset_leaves_no_pass_target(void)
{
	entity_player_tackled(g_passer_entity, g_opponent_entity, 1.0f, g_game_state);
	TEST_ASSERT_EQUAL_INT(PS_FALL, g_passer_entity->player.state_machine.current_state);
	g_passer_entity->player_controls.pass_target_entity_id = g_team_mate_entity->header.id;

	system_player_control_ai(g_game_state);

	TEST_ASSERT_EQUAL_UINT32(EID_NOT_VALID, g_passer_entity->player_controls.pass_target_entity_id);
}

// Headers

static v2 opponents_goal_center(void) { return g_game_state->vars.pitch.goal[PSI_SOUTH].center; }

static void header_on_the_ball(enum header_type type)
{
	entity_player_header_start(g_passer_entity, type, g_game_state);
	g_passer_entity->player.is_colliding_with_ball = true;
}

// Whether the header goes to a man or at goal was decided when he went up for it. The AI must not
// decide again from where he stands.
void test_an_ai_header_meant_as_a_pass_names_a_receiver_even_near_goal(void)
{
	g_passer_entity->transform.position = Vector2Subtract(opponents_goal_center(), (v2){.y = 150});
	place_relative_to_passer(g_team_mate_entity, 80, WIDE_ANGLE_DEG);
	header_on_the_ball(HEADER_TYPE_PASS);

	system_player_control_ai(g_game_state);

	TEST_ASSERT_EQUAL_UINT32(g_team_mate_entity->header.id,
							 g_passer_entity->player_controls.pass_target_entity_id);
}

void test_an_ai_header_meant_as_a_shot_names_nobody_even_far_from_goal(void)
{
	place_relative_to_passer(g_team_mate_entity, 80, WIDE_ANGLE_DEG);
	header_on_the_ball(HEADER_TYPE_SHOOT);

	system_player_control_ai(g_game_state);

	TEST_ASSERT_EQUAL_UINT32(EID_NOT_VALID, g_passer_entity->player_controls.pass_target_entity_id);
}

void test_a_header_goes_to_the_man_in_the_controls(void)
{
	place_relative_to_passer(g_team_mate_entity, 150, WIDE_ANGLE_DEG);
	header_on_the_ball(HEADER_TYPE_PASS);
	g_passer_entity->player.state_machine.state_data.header.phase = HP_AIR_1;
	entity_ball_set_position_and_velocity(g_ball_entity,
										  V2_TO_V3(g_passer_entity->transform.position,
												   header_phase_windows[HP_AIR_1 - 1].optimal),
										  Vector3Zero());
	passer_on_the_ball();
	g_passer_entity->player_controls.steering = UP_THE_PITCH;
	g_passer_entity->player_controls.pass_target_entity_id = g_team_mate_entity->header.id;

	system_player_on_the_ball(g_game_state);

	TEST_ASSERT_EQUAL_UINT32(g_team_mate_entity->header.id,
							 g_ball_entity->ball.pass_target_entity_id);
	TEST_ASSERT_FLOAT_WITHIN(
		0.001f, 1.0f, Vector2DotProduct(ball_direction(), direction_to(g_team_mate_entity)));
}

int main()
{
	GLOG_INFO(LS_TESTS, "passing test - start");

	UNITY_BEGIN();

	RUN_TEST(test_the_ai_search_gives_the_same_man_when_asked_twice);
	RUN_TEST(test_the_human_search_gives_the_same_man_when_asked_twice);
	RUN_TEST(test_the_searches_never_pick_an_opponent);
	RUN_TEST(test_the_searches_never_pick_a_man_outside_the_cone);
	RUN_TEST(test_the_searches_never_pick_the_passer);
	RUN_TEST(test_the_human_search_takes_the_man_at_the_charged_distance_even_behind);
	RUN_TEST(test_the_ai_search_prefers_the_man_up_the_pitch);
	RUN_TEST(test_the_ai_search_never_picks_a_man_out_of_range);
	RUN_TEST(test_the_ai_search_skips_a_screened_man_only_when_asked_to);
	RUN_TEST(test_the_ai_search_does_not_give_the_ball_straight_back);
	RUN_TEST(test_the_ai_search_gives_the_ball_back_once_the_pass_back_time_is_over);

	RUN_TEST(test_the_pass_goes_to_the_man_in_the_controls);
	RUN_TEST(test_an_automatic_pass_is_struck_along_the_ground);
	RUN_TEST(test_a_pass_with_nobody_chosen_goes_along_the_steering_to_nobody);
	RUN_TEST(test_a_man_running_at_the_touchline_is_led_short_of_it);
	RUN_TEST(test_a_pass_to_a_man_on_the_goal_line_aims_inside_the_field);
	RUN_TEST(test_a_man_already_outside_the_margin_is_aimed_at_from_inside_it);
	RUN_TEST(test_the_controls_reset_leaves_no_pass_target);

	RUN_TEST(test_an_ai_header_meant_as_a_pass_names_a_receiver_even_near_goal);
	RUN_TEST(test_an_ai_header_meant_as_a_shot_names_nobody_even_far_from_goal);
	RUN_TEST(test_a_header_goes_to_the_man_in_the_controls);

	return UNITY_END();
}
