#include "ai.h"
#include "asset_manager.h"
#include "entity.h"
#include "entity_ball.h"
#include "entity_components.h"
#include "entity_player.h"
#include "football.h"
#include "game.h"
#include "log.h"
#include "memory.h"
#include "types.h"
#include "unity/unity.h"
#include "unity/unity_internals.h"
#include <raylib.h>
#include <stdlib.h>

#define PLAY_TIME		 1.0
#define LATER_TOUCH_TIME 2.0

// When a keeper may take the ball in his hands. The answer is read off what the ball records about
// the last man to play it, so each test writes that record by hand and asks.

static struct game_state *g_game_state = NULL;
static void *g_memory = NULL;
static memory_arena_handle_t *g_main_memory_arena = NULL;

static struct entity *g_ball_entity = NULL;
static struct entity *g_goalie_entity = NULL;
static struct entity *g_team_mate_entity = NULL;
static struct entity *g_opponent_entity = NULL;

void setUp(void)
{
	const int width = 800;
	const int height = 600;
	InitWindow(width, height, "Test Goalkeeper");

	SetTargetFPS(60);

	g_memory = malloc(Megabytes(128));

	g_main_memory_arena = memory_arena_create_default("main", g_memory, Megabytes(128));

	struct assets assets = asset_manager_load_everything(g_main_memory_arena);

	struct game_config game_config =
		(struct game_config){.screen_width = width, .screen_height = height};

	g_game_state = game_play_init(game_config, assets, g_main_memory_arena);

	// A test build skips gameplay_init (game_play.c), so there is no match behind this: no ball, no
	// players, no sides and a clock at zero. The bits these rules read are set here.
	g_game_state->time = 10;
	g_game_state->gameplay.state_machine.current_state = FS_PLAY;
	g_game_state->vars.teams[TS_HOME].play_context.pitch_side = PSI_NORTH;
	g_game_state->vars.teams[TS_AWAY].play_context.pitch_side = PSI_SOUTH;

	g_ball_entity = entity_ball_create(
		(struct ball_config){.position = g_game_state->vars.pitch.midfield_spot}, g_game_state);
	g_game_state->vars.ball.entity = g_ball_entity;

	g_goalie_entity = entity_player_create(
		(struct player_config){.is_goalie = true, .team_side = TS_HOME}, g_game_state);
	g_team_mate_entity =
		entity_player_create((struct player_config){.team_side = TS_HOME}, g_game_state);
	g_opponent_entity =
		entity_player_create((struct player_config){.team_side = TS_AWAY}, g_game_state);
}

void tearDown(void)
{
	game_play_destroy(g_game_state);

	memory_arena_destroy(g_main_memory_arena);

	free(g_memory);

	CloseWindow();
}

// The ball as it is straight after somebody played it, nobody having touched it since: playing the
// ball is a touch of it too, so both are stamped with the same time.
static void ball_played_by(struct entity *player_entity, enum ball_play_type play_type)
{
	struct component_ball *ball = &g_ball_entity->ball;

	ball->last_play_entity_id = player_entity->header.id;
	ball->last_play_type = play_type;
	ball->last_play_time = PLAY_TIME;
	ball->last_touch_entity_id = player_entity->header.id;
	ball->last_touch_time = PLAY_TIME;
	// Playing the ball is also letting go of it, so nobody is in possession of it
	ball->first_player_possession_time = PLAY_TIME;
	ball->last_loose_time = PLAY_TIME;
}

// Somebody got to the ball after that play, which is a touch and no play of its own.
static void ball_touched_by(struct entity *player_entity)
{
	struct component_ball *ball = &g_ball_entity->ball;

	ball->last_touch_entity_id = player_entity->header.id;
	ball->last_touch_time = LATER_TOUCH_TIME;
}

static b32 may_handle(void)
{
	return entity_ball_may_goalie_handle(g_ball_entity, g_goalie_entity, g_game_state);
}

void test_a_ball_nobody_has_played_may_be_handled(void)
{
	struct component_ball *ball = &g_ball_entity->ball;
	ball->last_play_entity_id = EID_NOT_VALID;
	ball->last_touch_entity_id = EID_NOT_VALID;

	TEST_ASSERT_TRUE(may_handle());
}

void test_an_opponents_shot_may_be_handled(void)
{
	ball_played_by(g_opponent_entity, BPT_SHOT);

	TEST_ASSERT_TRUE(may_handle());
}

void test_a_team_mates_pass_may_not_be_handled(void)
{
	ball_played_by(g_team_mate_entity, BPT_PASS);

	TEST_ASSERT_FALSE(may_handle());
}

// The pass reaches him and he takes a touch with his feet: that touch is his own and must not read
// as somebody else having taken the ball on.
void test_a_team_mates_pass_may_not_be_handled_once_he_has_touched_it(void)
{
	ball_played_by(g_team_mate_entity, BPT_PASS);
	ball_touched_by(g_goalie_entity);

	TEST_ASSERT_FALSE(may_handle());
}

void test_a_team_mates_long_pass_may_not_be_handled(void)
{
	ball_played_by(g_team_mate_entity, BPT_LONG_PASS);

	TEST_ASSERT_FALSE(may_handle());
}

void test_a_team_mates_header_may_be_handled(void)
{
	ball_played_by(g_team_mate_entity, BPT_HEADER);

	TEST_ASSERT_TRUE(may_handle());
}

void test_a_team_mates_pass_may_be_handled_once_another_team_mate_takes_it_on(void)
{
	ball_played_by(g_team_mate_entity, BPT_PASS);
	struct entity *another_team_mate =
		entity_player_create((struct player_config){.team_side = TS_HOME}, g_game_state);
	ball_touched_by(another_team_mate);

	TEST_ASSERT_TRUE(may_handle());
}

void test_a_ball_he_kicked_out_of_his_hands_may_not_be_handled(void)
{
	ball_played_by(g_goalie_entity, BPT_LONG_PASS);
	g_ball_entity->ball.state_machine.previous_state = BS_GK_HANDS;

	TEST_ASSERT_FALSE(may_handle());
}

// He put it down to play it with his feet, so the ball was last played by whoever played it before
// he caught it.
void test_a_ball_he_put_down_may_not_be_handled(void)
{
	ball_played_by(g_opponent_entity, BPT_SHOT);
	ball_touched_by(g_goalie_entity);
	g_ball_entity->ball.state_machine.previous_state = BS_GK_HANDS;

	TEST_ASSERT_FALSE(may_handle());
}

void test_a_ball_he_let_go_of_may_be_handled_once_an_opponent_touches_it(void)
{
	ball_played_by(g_goalie_entity, BPT_LONG_PASS);
	g_ball_entity->ball.state_machine.previous_state = BS_GK_HANDS;
	ball_touched_by(g_opponent_entity);

	TEST_ASSERT_TRUE(may_handle());
}

// Whether he is the goalie right now or a field player, settled every tick before anything reads
// his state. Hands belong to the goalie; where the ball is decides which he is.

static v2 position_in_his_area(void)
{
	const rect_t penalty_box = g_game_state->vars.pitch.goal[PSI_NORTH].penalty_box;
	return (v2){.x = penalty_box.x + penalty_box.width * 0.5f,
				.y = penalty_box.y + penalty_box.height * 0.5f};
}

static v2 position_up_the_pitch(void)
{
	return g_game_state->vars.pitch.midfield_spot;
}

// Under control: touched a moment ago and not let go of since.
static void ball_controlled_by(struct entity *player_entity)
{
	struct component_ball *ball = &g_ball_entity->ball;

	ball->last_touch_entity_id = player_entity->header.id;
	ball->last_touch_time = g_game_state->time;
	ball->first_player_possession_time = g_game_state->time - 0.5;
	ball->last_loose_time = 0;
}

// Loose: it came off him this instant, so nobody is in possession of it.
static void ball_loose_off(struct entity *player_entity)
{
	struct component_ball *ball = &g_ball_entity->ball;

	ball->last_touch_entity_id = player_entity->header.id;
	ball->last_touch_time = g_game_state->time;
	ball->first_player_possession_time = g_game_state->time;
	ball->last_loose_time = g_game_state->time;
}

static void keeper_plays_out(void)
{
	state_machine_player_fire(&g_goalie_entity->player.state_machine, PT_STEP_OUT, g_game_state);
}

static void update_role(void)
{
	entity_player_goalie_update_role(g_goalie_entity, g_game_state);
}

void test_he_plays_out_when_his_side_has_the_ball_up_the_pitch(void)
{
	g_goalie_entity->transform.position = position_in_his_area();
	g_ball_entity->transform.position = position_up_the_pitch();
	ball_controlled_by(g_team_mate_entity);

	update_role();

	TEST_ASSERT_EQUAL_INT(PS_NORMAL, g_goalie_entity->player.state_machine.current_state);
}

// A cross dropping on his six yard box is no moment to be playing out, whoever has the ball.
void test_a_ball_in_his_area_keeps_him_in_goal(void)
{
	g_goalie_entity->transform.position = position_in_his_area();
	g_ball_entity->transform.position = position_in_his_area();
	ball_controlled_by(g_team_mate_entity);

	update_role();

	TEST_ASSERT_EQUAL_INT(PS_GOALIE, g_goalie_entity->player.state_machine.current_state);
}

void test_he_is_never_the_goalie_outside_his_area(void)
{
	g_goalie_entity->transform.position = position_up_the_pitch();
	g_ball_entity->transform.position = position_up_the_pitch();
	ball_loose_off(g_opponent_entity);

	update_role();

	TEST_ASSERT_EQUAL_INT(PS_NORMAL, g_goalie_entity->player.state_machine.current_state);
}

void test_he_goes_back_in_goal_once_the_opponents_have_the_ball(void)
{
	keeper_plays_out();
	g_goalie_entity->transform.position = position_in_his_area();
	g_ball_entity->transform.position = position_up_the_pitch();
	ball_controlled_by(g_opponent_entity);

	update_role();

	TEST_ASSERT_EQUAL_INT(PS_GOALIE, g_goalie_entity->player.state_machine.current_state);
}

// The rebound that started all this: a shot comes back off the bar and lands on him while he is
// playing out. His own touch reads as his side having the ball, which must not strand him outfield.
void test_a_ball_rebounding_onto_him_in_his_area_puts_him_back_in_goal(void)
{
	keeper_plays_out();
	g_goalie_entity->transform.position = position_in_his_area();
	g_ball_entity->transform.position = position_in_his_area();
	ball_played_by(g_opponent_entity, BPT_SHOT);
	ball_loose_off(g_goalie_entity);

	update_role();

	TEST_ASSERT_EQUAL_INT(PS_GOALIE, g_goalie_entity->player.state_machine.current_state);
}

// One he may not pick up is one he has to play with his feet, so it leaves him out of goal.
void test_a_back_pass_into_his_area_leaves_him_out_on_his_feet(void)
{
	keeper_plays_out();
	g_goalie_entity->transform.position = position_in_his_area();
	g_ball_entity->transform.position = position_in_his_area();
	ball_played_by(g_team_mate_entity, BPT_PASS);

	update_role();

	TEST_ASSERT_EQUAL_INT(PS_NORMAL, g_goalie_entity->player.state_machine.current_state);
}

// What he does with the ball once he has it in his hands. His goalkeeping is not decided here.

static void keeper_catches_the_ball(void)
{
	state_machine_goalie_fire(&g_goalie_entity->goalie.state_machine, GT_CATCH, g_game_state);
	g_goalie_entity->transform.position = position_in_his_area();
}

// He has had his moment to look up.
static void time_to_think_passes(void)
{
	g_game_state->time += GOALIE_AI_THINK_TIME * 2;
}

static enum goalie_ai_action goalie_decision(void)
{
	return ai_goalie_think(g_goalie_entity, g_game_state).action;
}

static void place_relative_to_keeper(struct entity *player_entity, f32 up_the_pitch)
{
	player_entity->transform.position =
		Vector2Add(g_goalie_entity->transform.position, (v2){.y = up_the_pitch});
}

void test_he_holds_on_to_it_until_he_has_looked_up(void)
{
	keeper_catches_the_ball();

	TEST_ASSERT_EQUAL_INT(GAA_HOLD, goalie_decision());
}

void test_closed_down_he_clears_it(void)
{
	keeper_catches_the_ball();
	time_to_think_passes();
	place_relative_to_keeper(g_opponent_entity, GOALIE_AI_PRESSURE_RADIUS * 0.5f);
	place_relative_to_keeper(g_team_mate_entity, GOALIE_AI_THROW_RANGE * 4);

	TEST_ASSERT_EQUAL_INT(GAA_KICK, goalie_decision());
}

void test_a_free_team_mate_in_range_gets_it_thrown_to_him(void)
{
	keeper_catches_the_ball();
	time_to_think_passes();
	place_relative_to_keeper(g_team_mate_entity, GOALIE_AI_THROW_RANGE * 0.6f);
	place_relative_to_keeper(g_opponent_entity, -GOALIE_AI_THROW_RANGE);

	TEST_ASSERT_EQUAL_INT(GAA_THROW, goalie_decision());
}

// An opponent stood on the line of the throw is reason enough not to make it.
void test_a_screened_team_mate_does_not_get_the_throw(void)
{
	keeper_catches_the_ball();
	time_to_think_passes();
	place_relative_to_keeper(g_team_mate_entity, GOALIE_AI_THROW_RANGE * 0.6f);
	place_relative_to_keeper(g_opponent_entity, GOALIE_AI_THROW_RANGE * 0.3f);

	TEST_ASSERT_NOT_EQUAL_INT(GAA_THROW, goalie_decision());
}

void test_with_nobody_near_him_he_puts_it_down(void)
{
	keeper_catches_the_ball();
	time_to_think_passes();
	place_relative_to_keeper(g_team_mate_entity, GOALIE_AI_THROW_RANGE * 4);
	place_relative_to_keeper(g_opponent_entity, GOALIE_AI_THROW_RANGE * 4);

	TEST_ASSERT_EQUAL_INT(GAA_PUT_DOWN, goalie_decision());
}

int main()
{
	GLOG_INFO(LS_TESTS, "goalkeeper test - start");

	UNITY_BEGIN();

	RUN_TEST(test_a_ball_nobody_has_played_may_be_handled);
	RUN_TEST(test_an_opponents_shot_may_be_handled);
	RUN_TEST(test_a_team_mates_pass_may_not_be_handled);
	RUN_TEST(test_a_team_mates_pass_may_not_be_handled_once_he_has_touched_it);
	RUN_TEST(test_a_team_mates_long_pass_may_not_be_handled);
	RUN_TEST(test_a_team_mates_header_may_be_handled);
	RUN_TEST(test_a_team_mates_pass_may_be_handled_once_another_team_mate_takes_it_on);
	RUN_TEST(test_a_ball_he_kicked_out_of_his_hands_may_not_be_handled);
	RUN_TEST(test_a_ball_he_put_down_may_not_be_handled);
	RUN_TEST(test_a_ball_he_let_go_of_may_be_handled_once_an_opponent_touches_it);

	RUN_TEST(test_he_plays_out_when_his_side_has_the_ball_up_the_pitch);
	RUN_TEST(test_a_ball_in_his_area_keeps_him_in_goal);
	RUN_TEST(test_he_is_never_the_goalie_outside_his_area);
	RUN_TEST(test_he_goes_back_in_goal_once_the_opponents_have_the_ball);
	RUN_TEST(test_a_ball_rebounding_onto_him_in_his_area_puts_him_back_in_goal);
	RUN_TEST(test_a_back_pass_into_his_area_leaves_him_out_on_his_feet);

	RUN_TEST(test_he_holds_on_to_it_until_he_has_looked_up);
	RUN_TEST(test_closed_down_he_clears_it);
	RUN_TEST(test_a_free_team_mate_in_range_gets_it_thrown_to_him);
	RUN_TEST(test_a_screened_team_mate_does_not_get_the_throw);
	RUN_TEST(test_with_nobody_near_him_he_puts_it_down);

	return UNITY_END();
}
