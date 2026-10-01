#include "asset_manager.h"
#include "entity.h"
#include "entity_ball.h"
#include "entity_components.h"
#include "entity_player.h"
#include "football.h"
#include "game.h"
#include "log.h"
#include "memory.h"
#include "system.h"
#include "types.h"
#include "constants.h"
#include "unity/unity.h"
#include "unity/unity_internals.h"
#include <raylib.h>
#include <raymath.h>
#include <stdlib.h>

static struct game_state *g_game_state = NULL;
static void *g_memory = NULL;
static memory_arena_handle_t *g_main_memory_arena = NULL;

static struct entity *g_ball_entity = NULL;
// Two home team mates, created in this order so the systems reach the first one first
static struct entity *g_first_tackler_entity = NULL;
static struct entity *g_second_tackler_entity = NULL;
static struct entity *g_carrier_entity = NULL;

void setUp(void)
{
	const int width = 800;
	const int height = 600;
	InitWindow(width, height, "Test Tackle");

	SetTargetFPS(60);

	g_memory = malloc(Megabytes(128));

	g_main_memory_arena = memory_arena_create_default("main", g_memory, Megabytes(128));

	struct assets assets = asset_manager_load_everything(g_main_memory_arena);

	struct game_config game_config =
		(struct game_config){.screen_width = width, .screen_height = height};

	g_game_state = game_play_init(game_config, assets, g_main_memory_arena);

	// A test build skips gameplay_init (game_play.c), so there is no match behind this: no ball, no
	// players and a clock at zero. The bits these tests read are set here.
	g_game_state->time = 10;
	g_game_state->gameplay.state_machine.current_state = FS_PLAY;

	const v2 spot = g_game_state->vars.pitch.midfield_spot;

	g_ball_entity = entity_ball_create((struct ball_config){.position = spot}, g_game_state);
	g_game_state->vars.ball.entity = g_ball_entity;

	g_first_tackler_entity =
		entity_player_create((struct player_config){.team_side = TS_HOME}, g_game_state);
	g_second_tackler_entity =
		entity_player_create((struct player_config){.team_side = TS_HOME}, g_game_state);
	g_carrier_entity =
		entity_player_create((struct player_config){.team_side = TS_AWAY}, g_game_state);

	// The carrier has had the ball at his feet for a while
	g_carrier_entity->transform.position = spot;
	g_ball_entity->transform.virtual_height = 0;
	g_ball_entity->ball.last_touch_entity_id = g_carrier_entity->header.id;
	g_ball_entity->ball.first_player_possession_time = 9.0f;
	g_ball_entity->ball.last_touch_time = 9.99f;
	g_ball_entity->ball.last_loose_time = 8.0f;

	// Both tacklers stand on top of him, so their PCT_TACKLE reaches his PCT_BODY whichever way
	// they face
	g_first_tackler_entity->transform.position = Vector2Add(spot, (v2){0, -2});
	g_second_tackler_entity->transform.position = Vector2Add(spot, (v2){2, 0});
	entity_player_update_colliders(g_first_tackler_entity);
	entity_player_update_colliders(g_second_tackler_entity);
	entity_player_update_colliders(g_carrier_entity);
}

void tearDown(void)
{
	game_play_destroy(g_game_state);

	memory_arena_destroy(g_main_memory_arena);

	free(g_memory);

	CloseWindow();
}

// What the control systems write when they send a player in
static void goes_in(struct entity *tackler_entity)
{
	tackler_entity->player_controls.tackle = true;
	tackler_entity->player_controls.tackle_victim_entity_id =
		entity_player_find_tackle_victim(tackler_entity, g_game_state);
}

void test_the_tackle_goes_in_on_the_man_in_the_controls(void)
{
	goes_in(g_first_tackler_entity);
	system_players_off_the_ball(g_game_state);

	TEST_ASSERT_EQUAL_INT(PS_TACKLE, g_first_tackler_entity->player.state_machine.current_state);
	TEST_ASSERT_EQUAL_UINT32(
		g_carrier_entity->header.id,
		g_first_tackler_entity->player.state_machine.state_data.tackle.victim_entity_id);
}

// The first tackler plays the ball in the same pass that sends his team mate in, after the team
// mate's controls chose the carrier. The team mate still goes in on the man his controls chose.
void test_a_team_mate_going_in_as_the_ball_is_won_keeps_his_victim(void)
{
	goes_in(g_first_tackler_entity);
	system_players_off_the_ball(g_game_state);

	g_game_state->time += 1.0 / 480.0;
	g_first_tackler_entity->player.is_colliding_with_ball = true;
	g_first_tackler_entity->player.collision_direction = (v2){0, 1};
	g_first_tackler_entity->player.collision_force = 1;
	goes_in(g_second_tackler_entity);
	TEST_ASSERT_EQUAL_UINT32(g_carrier_entity->header.id,
							 g_second_tackler_entity->player_controls.tackle_victim_entity_id);

	system_players_off_the_ball(g_game_state);

	TEST_ASSERT_TRUE(g_first_tackler_entity->player.state_machine.state_data.tackle.touched_ball);
	TEST_ASSERT_EQUAL_INT(PS_TACKLE, g_second_tackler_entity->player.state_machine.current_state);
	TEST_ASSERT_EQUAL_UINT32(
		g_carrier_entity->header.id,
		g_second_tackler_entity->player.state_machine.state_data.tackle.victim_entity_id);
}

void test_possession_is_playable_only_with_the_ball_at_his_feet(void)
{
	struct ball_possession possession =
		entity_ball_player_in_possession(g_ball_entity, g_game_state);
	TEST_ASSERT_EQUAL_UINT32(g_carrier_entity->header.id, possession.entity_id);
	TEST_ASSERT_EQUAL_INT(BPOS_PLAYABLE, possession.type);

	// Knocked on just past playing distance: still his, but out of reach
	g_ball_entity->transform.position = Vector2Add(
		g_carrier_entity->transform.position,
		(v2){0, g_carrier_entity->player.constants.control_radius + PLAYING_DISTANCE + 1});
	possession = entity_ball_player_in_possession(g_ball_entity, g_game_state);
	TEST_ASSERT_EQUAL_UINT32(g_carrier_entity->header.id, possession.entity_id);
	TEST_ASSERT_EQUAL_INT(BPOS_OUT_OF_REACH, possession.type);
}

// The first tackler slides in and plays the ball on this tick
static void slides_in_and_plays_the_ball(void)
{
	g_first_tackler_entity->player_controls.sliding_tackle = true;
	system_players_off_the_ball(g_game_state);
	TEST_ASSERT_EQUAL_INT(PS_SLIDING_TACKLE,
						  g_first_tackler_entity->player.state_machine.current_state);

	g_game_state->time += 1.0 / 480.0;
	g_game_state->vars.ball.possession_at_tick_start =
		entity_ball_player_in_possession(g_ball_entity, g_game_state);
	g_first_tackler_entity->player.collision_direction = (v2){0, 1};
	g_first_tackler_entity->player.collision_force = 1;
	entity_player_gained_possession(g_first_tackler_entity, g_game_state);
	system_player_on_the_ball(g_game_state);

	TEST_ASSERT_TRUE(
		g_first_tackler_entity->player.state_machine.state_data.sliding_tackle.touched_ball);
}

void test_a_slide_playing_the_ball_locks_the_man_who_had_it_at_his_feet(void)
{
	slides_in_and_plays_the_ball();

	TEST_ASSERT_EQUAL_FLOAT(g_game_state->time,
							g_carrier_entity->player.last_tackle_dispossession_time);
}

void test_a_slide_playing_a_ball_knocked_on_leaves_the_man_who_had_it_free(void)
{
	g_ball_entity->transform.position = Vector2Add(
		g_carrier_entity->transform.position,
		(v2){0, g_carrier_entity->player.constants.control_radius + PLAYING_DISTANCE + 1});

	slides_in_and_plays_the_ball();

	TEST_ASSERT_EQUAL_FLOAT(0, g_carrier_entity->player.last_tackle_dispossession_time);
}

// The first tackler's tackle leaves the carrier on his feet, in a tackled run
static void first_tackler_starts_a_tackled_run(void)
{
	entity_player_tackled(g_carrier_entity, g_first_tackler_entity, 0.0f, g_game_state);
	TEST_ASSERT_EQUAL_INT(PS_TACKLED_RUN, g_carrier_entity->player.state_machine.current_state);
}

void test_the_man_who_started_the_run_cannot_tackle_the_runner_again(void)
{
	first_tackler_starts_a_tackled_run();

	TEST_ASSERT_EQUAL_UINT32(
		EID_NOT_VALID, entity_player_find_tackle_victim(g_first_tackler_entity, g_game_state));
	TEST_ASSERT_EQUAL_UINT32(g_carrier_entity->header.id,
							 entity_player_find_tackle_victim(g_second_tackler_entity,
															  g_game_state));
}

void test_a_slide_does_not_hit_the_runner_it_started_again(void)
{
	first_tackler_starts_a_tackled_run();

	TEST_ASSERT_EQUAL_INT(
		0,
		entity_player_find_tackled_opponents(g_first_tackler_entity, PCT_TACKLE, g_game_state)
			.count);
	TEST_ASSERT_EQUAL_INT(
		1,
		entity_player_find_tackled_opponents(g_second_tackler_entity, PCT_TACKLE, g_game_state)
			.count);
}

void test_a_second_tackle_on_a_runner_knocks_him_down(void)
{
	first_tackler_starts_a_tackled_run();

	entity_player_tackled(g_carrier_entity, g_second_tackler_entity, 0.0f, g_game_state);

	TEST_ASSERT_EQUAL_INT(PS_FALL, g_carrier_entity->player.state_machine.current_state);
}

int main()
{
	GLOG_INFO(LS_TESTS, "tackle test - start");

	UNITY_BEGIN();

	RUN_TEST(test_the_tackle_goes_in_on_the_man_in_the_controls);
	RUN_TEST(test_a_team_mate_going_in_as_the_ball_is_won_keeps_his_victim);
	RUN_TEST(test_possession_is_playable_only_with_the_ball_at_his_feet);
	RUN_TEST(test_a_slide_playing_the_ball_locks_the_man_who_had_it_at_his_feet);
	RUN_TEST(test_a_slide_playing_a_ball_knocked_on_leaves_the_man_who_had_it_free);
	RUN_TEST(test_the_man_who_started_the_run_cannot_tackle_the_runner_again);
	RUN_TEST(test_a_slide_does_not_hit_the_runner_it_started_again);
	RUN_TEST(test_a_second_tackle_on_a_runner_knocks_him_down);

	GLOG_INFO(LS_TESTS, "tackle test - end");

	return UNITY_END();
}
