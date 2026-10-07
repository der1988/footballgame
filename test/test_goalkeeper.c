#include "ai.h"
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
#include <stdlib.h>

#define PLAY_TIME 1.0

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
	// A header leaves the goalie his hands, any other play bars them for the player's own goalie
	if (play_type == BPT_HEADER) {
		ball->not_pickable_by = BNPB_NOBODY;
	} else {
		ball->not_pickable_by = player_entity->player.team_side == TS_HOME ? BNPB_HOME_GOALIE :
																			 BNPB_AWAY_GOALIE;
	}
}

// Somebody got to the ball after that play, which is a touch and no play of its own.
static void ball_touched_by(struct entity *player_entity)
{
	entity_ball_set_last_touch(g_ball_entity, player_entity->header.id, g_game_state);
}

// He takes the ball in his hands and lets go of it again
static void keeper_lets_go_of_the_ball(void)
{
	entity_ball_held_by_goalie(g_ball_entity, g_goalie_entity->header.id, g_game_state);
	entity_ball_released_by_goalie(g_ball_entity, g_goalie_entity->header.id, g_game_state);
}

static b32 may_handle(void)
{
	return !entity_ball_is_goalie_hands_barred(g_ball_entity, g_goalie_entity->player.team_side);
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

// The passer running onto his own pass is still his own team, however the touch comes about
void test_a_team_mates_pass_may_not_be_handled_once_the_passer_touches_it_again(void)
{
	ball_played_by(g_team_mate_entity, BPT_PASS);
	ball_touched_by(g_team_mate_entity);

	TEST_ASSERT_FALSE(may_handle());
}

// A deflection off another team mate is a touch by his own team, so it changes nothing
void test_a_team_mates_pass_may_not_be_handled_once_another_team_mate_touches_it(void)
{
	ball_played_by(g_team_mate_entity, BPT_PASS);
	struct entity *another_team_mate =
		entity_player_create((struct player_config){.team_side = TS_HOME}, g_game_state);
	ball_touched_by(another_team_mate);

	TEST_ASSERT_FALSE(may_handle());
}

void test_a_team_mates_pass_may_be_handled_once_another_team_mate_heads_it(void)
{
	ball_played_by(g_team_mate_entity, BPT_PASS);
	struct entity *another_team_mate =
		entity_player_create((struct player_config){.team_side = TS_HOME}, g_game_state);
	ball_touched_by(another_team_mate);
	entity_ball_header(g_ball_entity, another_team_mate->header.id, Vector3Zero(), 0, g_game_state);

	TEST_ASSERT_TRUE(may_handle());
}

// The helpers above write the play by hand; this one goes through the pass itself
void test_a_pass_played_by_a_team_mate_bars_his_hands(void)
{
	ball_touched_by(g_team_mate_entity);
	entity_ball_short_pass(
		g_ball_entity, g_team_mate_entity->header.id, (v3){.z = 1}, 0.5f, g_game_state);

	TEST_ASSERT_FALSE(may_handle());
}

// His own header is not a team mate's
void test_a_team_mates_pass_may_not_be_handled_once_he_heads_it_himself(void)
{
	ball_played_by(g_team_mate_entity, BPT_PASS);
	ball_touched_by(g_goalie_entity);
	entity_ball_header(g_ball_entity, g_goalie_entity->header.id, Vector3Zero(), 0, g_game_state);

	TEST_ASSERT_FALSE(may_handle());
}

// Any touch by the other team, a deflection or a tackle as much as a control
void test_a_team_mates_pass_may_be_handled_once_an_opponent_touches_it(void)
{
	ball_played_by(g_team_mate_entity, BPT_PASS);
	ball_touched_by(g_opponent_entity);

	TEST_ASSERT_TRUE(may_handle());
}

// His own touch after the opponent's must not bring the old pass back
void test_a_team_mates_pass_touched_by_an_opponent_may_be_handled_after_he_touches_it_too(void)
{
	ball_played_by(g_team_mate_entity, BPT_PASS);
	ball_touched_by(g_opponent_entity);
	ball_touched_by(g_goalie_entity);

	TEST_ASSERT_TRUE(may_handle());
}

void test_a_ball_he_kicked_out_of_his_hands_may_not_be_handled(void)
{
	keeper_lets_go_of_the_ball();
	ball_played_by(g_goalie_entity, BPT_LONG_PASS);

	TEST_ASSERT_FALSE(may_handle());
}

// He put it down to play it with his feet, so the ball was last played by whoever played it before
// he caught it.
void test_a_ball_he_put_down_may_not_be_handled(void)
{
	ball_played_by(g_opponent_entity, BPT_SHOT);
	keeper_lets_go_of_the_ball();

	TEST_ASSERT_FALSE(may_handle());
}

void test_a_ball_he_let_go_of_may_be_handled_once_an_opponent_touches_it(void)
{
	keeper_lets_go_of_the_ball();
	ball_touched_by(g_opponent_entity);

	TEST_ASSERT_TRUE(may_handle());
}

void test_a_ball_he_let_go_of_may_not_be_handled_once_a_team_mate_touches_it(void)
{
	keeper_lets_go_of_the_ball();
	ball_touched_by(g_team_mate_entity);

	TEST_ASSERT_FALSE(may_handle());
}

// A parry is no release: once he has let go of the ball earlier, the rebound of a later shot he
// parries is still his to pick up.
void test_the_rebound_of_a_shot_he_parried_may_be_handled(void)
{
	keeper_lets_go_of_the_ball();
	ball_played_by(g_opponent_entity, BPT_SHOT);
	entity_ball_saved_by_goalie(g_ball_entity, g_goalie_entity->header.id, g_game_state);

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
	ball_played_by(g_team_mate_entity, BPT_PASS);
	ball_controlled_by(g_team_mate_entity);

	update_role();

	TEST_ASSERT_EQUAL_INT(PS_NORMAL, g_goalie_entity->player.state_machine.current_state);
}

void test_a_team_mate_on_the_ball_in_his_area_has_him_play_out(void)
{
	g_goalie_entity->transform.position = position_in_his_area();
	g_ball_entity->transform.position = position_in_his_area();
	ball_played_by(g_team_mate_entity, BPT_PASS);
	ball_controlled_by(g_team_mate_entity);

	update_role();

	TEST_ASSERT_EQUAL_INT(PS_NORMAL, g_goalie_entity->player.state_machine.current_state);
}

// An opponent's shot blocked and controlled by a team mate is one he may still handle
void test_a_blocked_shot_a_team_mate_controls_keeps_him_in_goal(void)
{
	g_goalie_entity->transform.position = position_in_his_area();
	g_ball_entity->transform.position = position_in_his_area();
	ball_played_by(g_opponent_entity, BPT_SHOT);
	ball_controlled_by(g_team_mate_entity);

	update_role();

	TEST_ASSERT_EQUAL_INT(PS_GOALIE, g_goalie_entity->player.state_machine.current_state);
}

// A back pass on its way to him steps him out before it gets there
void test_a_back_pass_steps_him_out_before_it_reaches_him(void)
{
	g_goalie_entity->transform.position = position_in_his_area();
	g_ball_entity->transform.position = position_up_the_pitch();
	ball_played_by(g_team_mate_entity, BPT_PASS);

	update_role();

	TEST_ASSERT_EQUAL_INT(PS_NORMAL, g_goalie_entity->player.state_machine.current_state);
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

// A header back to him arrives above control height, out of reach of the field player's ball test,
// so it has to be the goalie that catches it.
void test_a_team_mates_header_into_his_area_is_caught_while_playing_out(void)
{
	keeper_plays_out();
	g_goalie_entity->transform.position = position_in_his_area();
	ball_played_by(g_team_mate_entity, BPT_HEADER);
	entity_ball_set_position_and_velocity(g_ball_entity,
										  V2_TO_V3(g_goalie_entity->transform.position,
												   g_game_state->constants.crossbar_height - 5),
										  Vector3Zero());

	system_goalie(g_game_state);
	system_player_collisions(g_game_state);
	system_player_on_the_ball(g_game_state);

	TEST_ASSERT_EQUAL_INT(BS_GK_HANDS, g_ball_entity->ball.state_machine.current_state);
	TEST_ASSERT_EQUAL_INT(GS_HOLD_BALL, g_goalie_entity->goalie.state_machine.current_state);
}

// What he does with the ball once he has it in his hands.

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

	TEST_ASSERT_EQUAL_INT(GAA_NO_ACTION, goalie_decision());
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

// The man out of range scores higher on distance, and must not stop the throw to the man in range.
void test_a_man_out_of_range_does_not_stop_the_throw_to_a_man_in_range(void)
{
	keeper_catches_the_ball();
	time_to_think_passes();
	place_relative_to_keeper(g_team_mate_entity, GOALIE_AI_THROW_RANGE * 0.14f);
	struct entity *out_of_range_entity =
		entity_player_create((struct player_config){.team_side = TS_HOME}, g_game_state);
	place_relative_to_keeper(out_of_range_entity, GOALIE_AI_THROW_RANGE * 1.04f);
	place_relative_to_keeper(g_opponent_entity, -GOALIE_AI_THROW_RANGE);

	const struct goalie_ai_decision decision = ai_goalie_think(g_goalie_entity, g_game_state);

	TEST_ASSERT_EQUAL_INT(GAA_THROW, decision.action);
	TEST_ASSERT_EQUAL_UINT32(g_team_mate_entity->header.id,
							 decision.action_data.throw_ball.target_entity_id);
}

// The man he decides to throw to is named in the decision, so the throw cannot pick another.
void test_the_throw_names_the_team_mate_it_goes_to(void)
{
	keeper_catches_the_ball();
	time_to_think_passes();
	place_relative_to_keeper(g_team_mate_entity, GOALIE_AI_THROW_RANGE * 0.6f);
	place_relative_to_keeper(g_opponent_entity, -GOALIE_AI_THROW_RANGE);

	const struct goalie_ai_decision decision = ai_goalie_think(g_goalie_entity, g_game_state);

	TEST_ASSERT_EQUAL_INT(GAA_THROW, decision.action);
	TEST_ASSERT_EQUAL_UINT32(g_team_mate_entity->header.id,
							 decision.action_data.throw_ball.target_entity_id);
}

void test_with_nobody_near_him_he_puts_it_down(void)
{
	keeper_catches_the_ball();
	time_to_think_passes();
	place_relative_to_keeper(g_team_mate_entity, GOALIE_AI_THROW_RANGE * 4);
	place_relative_to_keeper(g_opponent_entity, GOALIE_AI_THROW_RANGE * 4);

	TEST_ASSERT_EQUAL_INT(GAA_PUT_DOWN, goalie_decision());
}

// The ball in his hands rides below control height, so every ball collision case would otherwise
// collide with it and hand the last touch to whoever walked past. Losing the touch is not cosmetic:
// the releases only run for the player entity_ball_player_on_the_ball returns, so a keeper who loses
// it is holding a ball he can never let go of.

static void keeper_picks_the_ball_up(void)
{
	keeper_plays_out();
	g_goalie_entity->transform.position = position_in_his_area();
	g_ball_entity->transform.position = position_in_his_area();

	TEST_ASSERT_TRUE(entity_player_goalie_try_pick_up_ball(g_goalie_entity, g_game_state));
	TEST_ASSERT_EQUAL_INT(BS_GK_HANDS, g_ball_entity->ball.state_machine.current_state);

	// While he holds it, system_ball rides the ball at his hands, below control height. That is
	// what puts it inside everybody else's collider, so the fixture has to place it there too: a
	// ball left at its creation height of 100 is out of everyone's reach and tests nothing.
	entity_ball_set_position_and_velocity(g_ball_entity,
										  V2_TO_V3(g_goalie_entity->transform.position,
												   GOALIE_HOLD_BALL_HEIGHT),
										  Vector3Zero());
}

// Stood on top of him, overlapping the ball, but not quite on the same spot as it.
static void opponent_closes_in_on_the_ball(void)
{
	g_opponent_entity->transform.position =
		Vector2Add(g_ball_entity->transform.position, (v2){.x = 2});
}

void test_an_opponent_cannot_take_a_ball_out_of_his_hands(void)
{
	keeper_picks_the_ball_up();
	opponent_closes_in_on_the_ball();

	system_player_collisions(g_game_state);

	TEST_ASSERT_FALSE(g_opponent_entity->player.is_colliding_with_ball);
	TEST_ASSERT_EQUAL_UINT32(g_goalie_entity->header.id, g_ball_entity->ball.last_touch_entity_id);
}

// And if anything does take the touch off him for a tick, he has to take it straight back: the stamp
// is keyed on him holding the ball, never on him already being the last man to touch it.
void test_he_reclaims_the_touch_if_something_takes_it_off_him(void)
{
	keeper_picks_the_ball_up();
	entity_ball_set_last_touch(g_ball_entity, g_opponent_entity->header.id, g_game_state);

	system_player_collisions(g_game_state);

	TEST_ASSERT_EQUAL_UINT32(g_goalie_entity->header.id, g_ball_entity->ball.last_touch_entity_id);
}

// A ball the keeper and an opponent reach on the same tick is the keeper's, even with the opponent
// nearer to it.
void test_a_ball_he_and_an_opponent_reach_together_is_his(void)
{
	entity_ball_set_position_and_velocity(
		g_ball_entity, V2_TO_V3(position_in_his_area(), GOALIE_HOLD_BALL_HEIGHT), Vector3Zero());
	g_opponent_entity->transform.position = g_ball_entity->transform.position;
	g_goalie_entity->transform.position =
		Vector2Add(g_ball_entity->transform.position, (v2){.x = 2});

	system_player_collisions(g_game_state);
	system_player_on_the_ball(g_game_state);

	TEST_ASSERT_EQUAL_INT(BS_GK_HANDS, g_ball_entity->ball.state_machine.current_state);
	TEST_ASSERT_EQUAL_INT(GS_HOLD_BALL, g_goalie_entity->goalie.state_machine.current_state);
	TEST_ASSERT_EQUAL_UINT32(g_goalie_entity->header.id, g_ball_entity->ball.last_touch_entity_id);
}

// The throw itself, not just the decision: the ball has to go to the man the AI chose. The second
// team mate stands straight up the pitch, where a search aimed only up the pitch would find him
// instead of the man the AI chose out wide.
void test_the_ball_is_thrown_to_the_team_mate_he_chose(void)
{
	keeper_picks_the_ball_up();
	time_to_think_passes();
	g_team_mate_entity->transform.position =
		Vector2Add(g_goalie_entity->transform.position,
				   Vector2Rotate((v2){.y = GOALIE_AI_THROW_RANGE * 0.6f}, 45 * DEG2RAD));
	struct entity *team_mate_up_the_pitch_entity =
		entity_player_create((struct player_config){.team_side = TS_HOME}, g_game_state);
	place_relative_to_keeper(team_mate_up_the_pitch_entity, GOALIE_AI_THROW_RANGE * 0.95f);
	place_relative_to_keeper(g_opponent_entity, -GOALIE_AI_THROW_RANGE);

	// In game_play.c order: the collisions stamp his touch again after the time he spent looking up
	system_player_collisions(g_game_state);
	system_player_control_ai(g_game_state);
	TEST_ASSERT_EQUAL_UINT32(g_team_mate_entity->header.id,
							 g_goalie_entity->player_controls.pass_target_entity_id);

	system_player_on_the_ball(g_game_state);

	TEST_ASSERT_EQUAL_INT(GS_THROW, g_goalie_entity->goalie.state_machine.current_state);
	TEST_ASSERT_EQUAL_UINT32(g_team_mate_entity->header.id,
							 g_ball_entity->ball.pass_target_entity_id);
}

// A throw with nobody chosen goes where he aims it, and nobody is told to run onto it.
void test_a_throw_with_nobody_chosen_has_no_receiver(void)
{
	keeper_picks_the_ball_up();
	place_relative_to_keeper(g_team_mate_entity, GOALIE_AI_THROW_RANGE * 0.6f);
	g_goalie_entity->player_controls.pass = 0.5f;
	g_goalie_entity->player_controls.pass_target_entity_id = EID_NOT_VALID;

	system_player_on_the_ball(g_game_state);

	TEST_ASSERT_EQUAL_INT(GS_THROW, g_goalie_entity->goalie.state_machine.current_state);
	TEST_ASSERT_EQUAL_UINT32(EID_NOT_VALID, g_ball_entity->ball.pass_target_entity_id);
}

// A dive that catches the ball puts it in BS_GK_HANDS while he is still in GS_DIVE. The carry in
// system_player_physics must wait for GS_HOLD_BALL: run during the dive, it swaps the dive's speed
// cap for the carry's and pushes him along with whatever steering is in his controls. The dive
// itself carries on sideways.
void test_a_dive_that_catches_the_ball_is_not_moved_by_the_carry(void)
{
	struct component_goalie *goalie = &g_goalie_entity->goalie;
	struct component_physics *physics = &g_goalie_entity->physics;

	g_goalie_entity->transform.position = position_in_his_area();
	entity_player_goalie_dive_start(
		g_goalie_entity, (struct goalie_dive){.direction = SD_RIGHT, .speed = 1.0f}, g_game_state);

	physics->velocity = (v2){.x = physics->max_velocity};
	const f32 dive_max_velocity = physics->max_velocity;
	const v2 dive_velocity = physics->velocity;
	const v2 dive_position = g_goalie_entity->transform.position;

	entity_ball_held_by_goalie(g_ball_entity, g_goalie_entity->header.id, g_game_state);
	TEST_ASSERT_EQUAL_INT(BS_GK_HANDS, g_ball_entity->ball.state_machine.current_state);
	g_goalie_entity->player_controls.steering = (v2){.y = 1};

	system_player_physics(g_game_state);

	TEST_ASSERT_EQUAL_INT(GS_DIVE, goalie->state_machine.current_state);
	TEST_ASSERT_EQUAL_FLOAT(dive_max_velocity, physics->max_velocity);
	TEST_ASSERT_EQUAL_FLOAT(dive_velocity.x, physics->velocity.x);
	TEST_ASSERT_EQUAL_FLOAT(dive_velocity.y, physics->velocity.y);
	TEST_ASSERT_GREATER_THAN_FLOAT(dive_position.x, g_goalie_entity->transform.position.x);
	TEST_ASSERT_EQUAL_FLOAT(dive_position.y, g_goalie_entity->transform.position.y);
}

// His goalkeeping: what ai_goalie_think decides while he is in goal without the ball. Only
// system_ball computes the ball's predictions, so each test writes the ones it needs by hand.

static void keeper_in_goal(void)
{
	g_goalie_entity->transform.position = position_in_his_area();
	TEST_ASSERT_EQUAL_INT(PS_GOALIE, g_goalie_entity->player.state_machine.current_state);
	TEST_ASSERT_EQUAL_INT(GS_PROTECT_GOAL, g_goalie_entity->goalie.state_machine.current_state);
}

static void ball_at(v2 position, f32 height, v3 velocity)
{
	entity_ball_set_position_and_velocity(g_ball_entity, V2_TO_V3(position, height), velocity);
}

static void ball_can_next_be_reached_at(v2 position, f32 height, u32 dts)
{
	g_ball_entity->ball.future_positions[FUTURE_POSITION_REACHABLE_TO_GOALIE] =
		(struct future_position){
			.available = true, .position = V2_TO_V3(position, height), .dts = dts};
}

static void ball_lookaheads_at(v2 position, f32 height)
{
	for (u16 i = 0; i < BALL_MAX_LOOKAHEADS; ++i) {
		g_game_state->vars.ball.lookahead_positions[i] = V2_TO_V3(position, height);
	}
}

static struct goalie_ai_decision keeper_thinks(void)
{
	return ai_goalie_think(g_goalie_entity, g_game_state);
}

void test_a_high_ball_coming_down_on_him_has_him_jump(void)
{
	keeper_in_goal();
	ball_played_by(g_opponent_entity, BPT_SHOT);
	ball_at(g_goalie_entity->transform.position,
			g_game_state->constants.goalie_max_reachable_height + 10,
			Vector3Zero());
	ball_can_next_be_reached_at(g_goalie_entity->transform.position, 20, 10);

	TEST_ASSERT_EQUAL_INT(GAA_JUMP, keeper_thinks().action);
}

void test_a_high_ball_he_may_not_handle_does_not_have_him_jump(void)
{
	keeper_in_goal();
	ball_played_by(g_team_mate_entity, BPT_PASS);
	ball_at(g_goalie_entity->transform.position,
			g_game_state->constants.goalie_max_reachable_height + 10,
			Vector3Zero());
	ball_can_next_be_reached_at(g_goalie_entity->transform.position, 20, 10);

	TEST_ASSERT_NOT_EQUAL_INT(GAA_JUMP, keeper_thinks().action);
}

// Just in front of him and going away sideways, fast: he has to dive to get to it
static void shot_going_past_him(void)
{
	const v2 ball_position = Vector2Add(g_goalie_entity->transform.position, (v2){.x = 30, .y = 3});
	ball_at(ball_position, 5, (v3){.x = 20 * PIXELS_PER_METER});
	ball_lookaheads_at(ball_position, 5);
}

void test_a_shot_going_past_him_has_him_dive_after_it(void)
{
	keeper_in_goal();
	ball_played_by(g_opponent_entity, BPT_SHOT);
	shot_going_past_him();

	const struct goalie_ai_decision decision = keeper_thinks();

	TEST_ASSERT_EQUAL_INT(GAA_DIVE, decision.action);
	TEST_ASSERT_EQUAL_INT(SD_RIGHT, decision.action_data.dive.direction);
}

void test_a_ball_he_may_not_handle_does_not_have_him_dive(void)
{
	keeper_in_goal();
	ball_played_by(g_team_mate_entity, BPT_PASS);
	shot_going_past_him();

	TEST_ASSERT_NOT_EQUAL_INT(GAA_DIVE, keeper_thinks().action);
}

void test_a_slow_loose_ball_in_his_area_has_him_come_for_it(void)
{
	keeper_in_goal();
	ball_played_by(g_opponent_entity, BPT_SHOT);
	const v2 ball_position = Vector2Add(g_goalie_entity->transform.position, (v2){.y = 20});
	ball_at(ball_position, 0, Vector3Zero());
	ball_can_next_be_reached_at(ball_position, 0, 60);
	ball_lookaheads_at(ball_position, 0);

	TEST_ASSERT_EQUAL_INT(GAS_CLAIM_BALL, keeper_thinks().state);
}

void test_with_an_opponent_on_the_ball_up_the_pitch_he_stands_in_goal(void)
{
	keeper_in_goal();
	ball_at(position_up_the_pitch(), 0, Vector3Zero());
	ball_controlled_by(g_opponent_entity);
	ball_lookaheads_at(position_up_the_pitch(), 0);

	TEST_ASSERT_EQUAL_INT(GAS_STAND, keeper_thinks().state);
}

void test_a_ball_about_to_cross_his_line_has_him_cover_it(void)
{
	keeper_in_goal();
	ball_played_by(g_opponent_entity, BPT_SHOT);
	const v2 goal_center = g_game_state->vars.pitch.goal[PSI_NORTH].center;
	ball_at(Vector2Add(goal_center, (v2){.x = 60, .y = 100}), 0, Vector3Zero());
	ball_lookaheads_at(Vector2Add(goal_center, (v2){.x = 20, .y = -10}), 0);

	TEST_ASSERT_EQUAL_INT(GAS_COVER_LINE, keeper_thinks().state);
}

void test_a_dribbler_heading_across_his_line_leaves_him_on_the_arc(void)
{
	keeper_in_goal();
	const v2 goal_center = g_game_state->vars.pitch.goal[PSI_NORTH].center;
	const v2 ball_position = Vector2Add(goal_center, (v2){.x = 60, .y = 100});
	ball_at(ball_position, 0, Vector3Zero());
	g_opponent_entity->transform.position = ball_position;
	ball_controlled_by(g_opponent_entity);
	ball_lookaheads_at(Vector2Add(goal_center, (v2){.x = 20, .y = -10}), 0);

	TEST_ASSERT_EQUAL_INT(GAS_STAND, keeper_thinks().state);
}

// Once he has let go of the ball, the ball's previous state stays BS_GK_HANDS until the next catch.
// A shot at him after that is still his to catch, not one he let go of himself.
void test_a_shot_after_he_has_let_go_of_the_ball_once_is_caught(void)
{
	keeper_in_goal();
	entity_ball_held_by_goalie(g_ball_entity, g_goalie_entity->header.id, g_game_state);
	entity_ball_released_by_goalie(g_ball_entity, g_goalie_entity->header.id, g_game_state);
	ball_played_by(g_opponent_entity, BPT_SHOT);
	ball_at(g_goalie_entity->transform.position, GOALIE_HOLD_BALL_HEIGHT, Vector3Zero());

	system_player_collisions(g_game_state);
	system_player_on_the_ball(g_game_state);

	TEST_ASSERT_EQUAL_INT(PS_GOALIE, g_goalie_entity->player.state_machine.current_state);
	TEST_ASSERT_EQUAL_INT(BS_GK_HANDS, g_ball_entity->ball.state_machine.current_state);
}

// A team mate's pass that reaches him in goal is not his to pick up: he steps out to play it.
void test_a_team_mates_pass_that_reaches_him_in_goal_has_him_step_out(void)
{
	keeper_in_goal();
	ball_played_by(g_team_mate_entity, BPT_PASS);
	ball_at(g_goalie_entity->transform.position, GOALIE_HOLD_BALL_HEIGHT, Vector3Zero());

	system_goalie(g_game_state);
	system_player_collisions(g_game_state);
	system_player_on_the_ball(g_game_state);

	TEST_ASSERT_EQUAL_INT(PS_NORMAL, g_goalie_entity->player.state_machine.current_state);
	TEST_ASSERT_EQUAL_INT(BS_IN_PLAY, g_ball_entity->ball.state_machine.current_state);
}

// Who fills the keeper's controls: the AI while he is in goal, even for a human, and the human once
// he has the ball in his hands.

static void keeper_controlled_by_a_human(void)
{
	g_game_state->human_players[0] =
		(struct human_player){.enabled = true,
							  .team_side = TS_HOME,
							  .controlled_player.entity_id = g_goalie_entity->header.id};
}

void test_a_human_keeper_in_goal_is_moved_by_the_ai(void)
{
	keeper_in_goal();
	keeper_controlled_by_a_human();
	ball_at(position_up_the_pitch(), 0, Vector3Zero());
	ball_controlled_by(g_opponent_entity);
	ball_lookaheads_at(position_up_the_pitch(), 0);

	system_player_control_human(g_game_state);
	system_player_control_ai(g_game_state);

	TEST_ASSERT_GREATER_THAN_FLOAT(0, Vector2Length(g_goalie_entity->player_controls.steering));
}

void test_a_human_keeper_holding_the_ball_is_left_to_the_human(void)
{
	keeper_catches_the_ball();
	keeper_controlled_by_a_human();
	time_to_think_passes();
	// Only a human winds up a throw, so nothing but his controls could have set this
	g_goalie_entity->player_controls.charging_throw = true;

	system_player_control_ai(g_game_state);

	TEST_ASSERT_TRUE(g_goalie_entity->player_controls.charging_throw);
}

// What physics does with the goalkeeping controls

void test_the_jump_control_takes_him_up(void)
{
	keeper_in_goal();
	g_goalie_entity->player_controls.jump = true;

	system_player_physics(g_game_state);

	TEST_ASSERT_EQUAL_INT(GS_JUMP, g_goalie_entity->goalie.state_machine.current_state);
}

void test_the_dive_control_takes_him_into_the_dive_it_describes(void)
{
	keeper_in_goal();
	g_goalie_entity->player_controls.dive = true;
	g_goalie_entity->player_controls.dive_data =
		(struct goalie_dive){.direction = SD_LEFT, .height = SH_LOW, .speed = 0.5f};

	system_player_physics(g_game_state);

	TEST_ASSERT_EQUAL_INT(GS_DIVE, g_goalie_entity->goalie.state_machine.current_state);
	TEST_ASSERT_EQUAL_INT(SD_LEFT,
						  g_goalie_entity->goalie.state_machine.state_data.dive.dive.direction);
	TEST_ASSERT_EQUAL_FLOAT(0.5f * g_game_state->constants.goalie_dive_max_velocity,
							g_goalie_entity->physics.max_velocity);
}

// Throwing himself on the ball

// On the ground, a short lunge up the pitch from him
static v2 ball_within_a_lunge(void)
{
	const v2 ball_position = Vector2Add(g_goalie_entity->transform.position, (v2){.y = 15});
	ball_at(ball_position, 0, Vector3Zero());
	ball_can_next_be_reached_at(ball_position, 0, 0);
	ball_lookaheads_at(ball_position, 0);
	return ball_position;
}

void test_an_opponent_dribbling_up_to_him_has_him_throw_himself_on_the_ball(void)
{
	keeper_in_goal();
	g_opponent_entity->transform.position = ball_within_a_lunge();
	ball_controlled_by(g_opponent_entity);

	const struct goalie_ai_decision decision = keeper_thinks();

	TEST_ASSERT_EQUAL_INT(GAA_SMOTHER, decision.action);
	TEST_ASSERT_EQUAL_FLOAT(0, decision.action_data.smother.direction.x);
	TEST_ASSERT_EQUAL_FLOAT(1, decision.action_data.smother.direction.y);
}

// The ball rolling on at the pace it has, the way system_ball would predict it
static void ball_moving_from(v2 position, v2 velocity)
{
	ball_at(position, 0, (v3){.x = velocity.x, .z = velocity.y});
	const f32 lookahead_time = g_game_state->dt * g_game_state->constants.ball_lookahead_dts;
	for (u16 i = 0; i < BALL_MAX_LOOKAHEADS; ++i) {
		g_game_state->vars.ball.lookahead_positions[i] =
			V2_TO_V3(Vector2Add(position, Vector2Scale(velocity, (i + 1) * lookahead_time)), 0);
	}
}

void test_he_throws_himself_at_where_a_moving_ball_will_be(void)
{
	keeper_in_goal();
	const v2 ball_position = ball_within_a_lunge();
	ball_moving_from(ball_position, (v2){.x = 3 * PIXELS_PER_METER});
	// The dribbler runs behind the ball, off the line the keeper takes to it
	g_opponent_entity->transform.position = Vector2Add(ball_position, (v2){.x = -10});
	ball_controlled_by(g_opponent_entity);

	const struct goalie_ai_decision decision = keeper_thinks();

	TEST_ASSERT_EQUAL_INT(GAA_SMOTHER, decision.action);
	TEST_ASSERT_GREATER_THAN_FLOAT(0, decision.action_data.smother.direction.x);
}

void test_an_opponent_between_him_and_the_ball_keeps_him_on_his_feet(void)
{
	keeper_in_goal();
	const v2 ball_position = ball_within_a_lunge();
	g_opponent_entity->transform.position =
		Vector2Lerp(g_goalie_entity->transform.position, ball_position, 0.5f);
	ball_controlled_by(g_opponent_entity);

	TEST_ASSERT_NOT_EQUAL_INT(GAA_SMOTHER, keeper_thinks().action);
}

void test_a_loose_ball_an_opponent_is_about_to_reach_has_him_throw_himself_on_it(void)
{
	keeper_in_goal();
	ball_played_by(g_opponent_entity, BPT_SHOT);
	const v2 ball_position = ball_within_a_lunge();
	g_opponent_entity->transform.position = Vector2Add(ball_position, (v2){.y = 15});

	TEST_ASSERT_EQUAL_INT(GAA_SMOTHER, keeper_thinks().action);
}

void test_a_loose_ball_with_nobody_near_it_is_not_one_to_throw_himself_on(void)
{
	keeper_in_goal();
	ball_played_by(g_opponent_entity, BPT_SHOT);
	ball_within_a_lunge();
	g_opponent_entity->transform.position = position_up_the_pitch();

	TEST_ASSERT_NOT_EQUAL_INT(GAA_SMOTHER, keeper_thinks().action);
}

void test_a_team_mate_on_the_ball_next_to_him_is_left_alone(void)
{
	keeper_in_goal();
	g_team_mate_entity->transform.position = ball_within_a_lunge();
	g_opponent_entity->transform.position = position_up_the_pitch();
	ball_controlled_by(g_team_mate_entity);

	TEST_ASSERT_NOT_EQUAL_INT(GAA_SMOTHER, keeper_thinks().action);
}

void test_a_ball_he_may_not_handle_is_not_one_to_throw_himself_on(void)
{
	keeper_in_goal();
	ball_played_by(g_team_mate_entity, BPT_PASS);
	const v2 ball_position = ball_within_a_lunge();
	g_opponent_entity->transform.position = Vector2Add(ball_position, (v2){.y = 15});

	TEST_ASSERT_NOT_EQUAL_INT(GAA_SMOTHER, keeper_thinks().action);
}

void test_the_smother_control_throws_him_along_its_direction(void)
{
	keeper_in_goal();
	const v2 start_position = g_goalie_entity->transform.position;
	g_goalie_entity->player_controls.smother = true;
	g_goalie_entity->player_controls.smother_direction = (v2){.y = 1};

	system_player_physics(g_game_state);
	TEST_ASSERT_EQUAL_INT(GS_SMOTHER, g_goalie_entity->goalie.state_machine.current_state);

	g_goalie_entity->player_controls.smother = false;
	system_player_physics(g_game_state);
	TEST_ASSERT_GREATER_THAN_FLOAT(start_position.y, g_goalie_entity->transform.position.y);
	TEST_ASSERT_EQUAL_FLOAT(start_position.x, g_goalie_entity->transform.position.x);
}

void test_landing_on_the_ball_puts_it_in_his_hands(void)
{
	keeper_in_goal();
	ball_at(Vector2Add(g_goalie_entity->transform.position, (v2){.y = 8}), 0, Vector3Zero());
	entity_player_goalie_smother_start(g_goalie_entity, (v2){.y = 1}, g_game_state);

	TEST_ASSERT_TRUE(entity_player_goalie_is_colliding_with_ball(g_goalie_entity, g_game_state));
	entity_player_goalie_on_the_ball(g_goalie_entity, g_game_state);

	TEST_ASSERT_EQUAL_INT(BS_GK_HANDS, g_ball_entity->ball.state_machine.current_state);
	TEST_ASSERT_FALSE(entity_player_goalie_is_colliding_with_ball(g_goalie_entity, g_game_state));
}

void test_an_opponent_he_hits_on_the_way_is_tackled(void)
{
	keeper_in_goal();
	g_opponent_entity->transform.position =
		Vector2Add(g_goalie_entity->transform.position, (v2){.y = 10});
	entity_player_goalie_smother_start(g_goalie_entity, (v2){.y = 1}, g_game_state);

	system_player_collisions(g_game_state);

	// Down or stumbling on, as chance has it
	TEST_ASSERT_NOT_EQUAL_INT(PS_NORMAL, g_opponent_entity->player.state_machine.current_state);
}

void test_an_opponent_out_of_his_reach_stays_on_his_feet(void)
{
	keeper_in_goal();
	g_opponent_entity->transform.position =
		Vector2Add(g_goalie_entity->transform.position, (v2){.y = 20});
	entity_player_goalie_smother_start(g_goalie_entity, (v2){.y = 1}, g_game_state);

	system_player_collisions(g_game_state);

	TEST_ASSERT_EQUAL_INT(PS_NORMAL, g_opponent_entity->player.state_machine.current_state);
}

void test_a_ball_barred_to_his_hands_after_he_has_gone_for_it_is_still_caught(void)
{
	keeper_in_goal();
	ball_at(Vector2Add(g_goalie_entity->transform.position, (v2){.y = 8}), 0, Vector3Zero());
	entity_player_goalie_smother_start(g_goalie_entity, (v2){.y = 1}, g_game_state);
	ball_played_by(g_team_mate_entity, BPT_PASS);
	TEST_ASSERT_FALSE(may_handle());

	entity_player_goalie_on_the_ball(g_goalie_entity, g_game_state);

	TEST_ASSERT_EQUAL_INT(BS_GK_HANDS, g_ball_entity->ball.state_machine.current_state);
}

void test_he_gets_up_from_the_ball_holding_it(void)
{
	keeper_in_goal();
	entity_player_goalie_smother_start(g_goalie_entity, (v2){.y = 1}, g_game_state);
	entity_ball_held_by_goalie(g_ball_entity, g_goalie_entity->header.id, g_game_state);
	g_goalie_entity->goalie.state_machine.state_data.smother.ball_held = true;

	g_game_state->time += 2;
	system_player_physics(g_game_state);

	TEST_ASSERT_EQUAL_INT(GS_HOLD_BALL, g_goalie_entity->goalie.state_machine.current_state);
	TEST_ASSERT_EQUAL_FLOAT(g_goalie_entity->player.constants.run_max_velocity,
							g_goalie_entity->physics.max_velocity);
}

void test_missing_the_ball_puts_him_back_on_his_feet_in_goal(void)
{
	keeper_in_goal();
	entity_player_goalie_smother_start(g_goalie_entity, (v2){.y = 1}, g_game_state);

	g_game_state->time += 2;
	system_player_physics(g_game_state);

	TEST_ASSERT_EQUAL_INT(GS_PROTECT_GOAL, g_goalie_entity->goalie.state_machine.current_state);
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
	RUN_TEST(test_a_team_mates_pass_may_not_be_handled_once_the_passer_touches_it_again);
	RUN_TEST(test_a_team_mates_pass_may_not_be_handled_once_another_team_mate_touches_it);
	RUN_TEST(test_a_team_mates_pass_may_be_handled_once_another_team_mate_heads_it);
	RUN_TEST(test_a_pass_played_by_a_team_mate_bars_his_hands);
	RUN_TEST(test_a_team_mates_pass_may_not_be_handled_once_he_heads_it_himself);
	RUN_TEST(test_a_team_mates_pass_may_be_handled_once_an_opponent_touches_it);
	RUN_TEST(test_a_team_mates_pass_touched_by_an_opponent_may_be_handled_after_he_touches_it_too);
	RUN_TEST(test_a_ball_he_kicked_out_of_his_hands_may_not_be_handled);
	RUN_TEST(test_a_ball_he_put_down_may_not_be_handled);
	RUN_TEST(test_a_ball_he_let_go_of_may_be_handled_once_an_opponent_touches_it);
	RUN_TEST(test_a_ball_he_let_go_of_may_not_be_handled_once_a_team_mate_touches_it);
	RUN_TEST(test_the_rebound_of_a_shot_he_parried_may_be_handled);

	RUN_TEST(test_he_plays_out_when_his_side_has_the_ball_up_the_pitch);
	RUN_TEST(test_a_team_mate_on_the_ball_in_his_area_has_him_play_out);
	RUN_TEST(test_a_blocked_shot_a_team_mate_controls_keeps_him_in_goal);
	RUN_TEST(test_a_back_pass_steps_him_out_before_it_reaches_him);
	RUN_TEST(test_he_is_never_the_goalie_outside_his_area);
	RUN_TEST(test_he_goes_back_in_goal_once_the_opponents_have_the_ball);
	RUN_TEST(test_a_ball_rebounding_onto_him_in_his_area_puts_him_back_in_goal);
	RUN_TEST(test_a_back_pass_into_his_area_leaves_him_out_on_his_feet);
	RUN_TEST(test_a_team_mates_header_into_his_area_is_caught_while_playing_out);

	RUN_TEST(test_he_holds_on_to_it_until_he_has_looked_up);
	RUN_TEST(test_closed_down_he_clears_it);
	RUN_TEST(test_a_free_team_mate_in_range_gets_it_thrown_to_him);
	RUN_TEST(test_a_screened_team_mate_does_not_get_the_throw);
	RUN_TEST(test_a_man_out_of_range_does_not_stop_the_throw_to_a_man_in_range);
	RUN_TEST(test_the_throw_names_the_team_mate_it_goes_to);
	RUN_TEST(test_with_nobody_near_him_he_puts_it_down);

	RUN_TEST(test_an_opponent_cannot_take_a_ball_out_of_his_hands);
	RUN_TEST(test_he_reclaims_the_touch_if_something_takes_it_off_him);
	RUN_TEST(test_a_ball_he_and_an_opponent_reach_together_is_his);

	RUN_TEST(test_the_ball_is_thrown_to_the_team_mate_he_chose);
	RUN_TEST(test_a_throw_with_nobody_chosen_has_no_receiver);

	RUN_TEST(test_a_dive_that_catches_the_ball_is_not_moved_by_the_carry);

	RUN_TEST(test_a_high_ball_coming_down_on_him_has_him_jump);
	RUN_TEST(test_a_high_ball_he_may_not_handle_does_not_have_him_jump);
	RUN_TEST(test_a_shot_going_past_him_has_him_dive_after_it);
	RUN_TEST(test_a_ball_he_may_not_handle_does_not_have_him_dive);
	RUN_TEST(test_a_slow_loose_ball_in_his_area_has_him_come_for_it);
	RUN_TEST(test_with_an_opponent_on_the_ball_up_the_pitch_he_stands_in_goal);
	RUN_TEST(test_a_ball_about_to_cross_his_line_has_him_cover_it);
	RUN_TEST(test_a_dribbler_heading_across_his_line_leaves_him_on_the_arc);
	RUN_TEST(test_a_shot_after_he_has_let_go_of_the_ball_once_is_caught);
	RUN_TEST(test_a_team_mates_pass_that_reaches_him_in_goal_has_him_step_out);

	RUN_TEST(test_a_human_keeper_in_goal_is_moved_by_the_ai);
	RUN_TEST(test_a_human_keeper_holding_the_ball_is_left_to_the_human);

	RUN_TEST(test_the_jump_control_takes_him_up);
	RUN_TEST(test_the_dive_control_takes_him_into_the_dive_it_describes);

	RUN_TEST(test_an_opponent_dribbling_up_to_him_has_him_throw_himself_on_the_ball);
	RUN_TEST(test_he_throws_himself_at_where_a_moving_ball_will_be);
	RUN_TEST(test_an_opponent_between_him_and_the_ball_keeps_him_on_his_feet);
	RUN_TEST(test_a_loose_ball_an_opponent_is_about_to_reach_has_him_throw_himself_on_it);
	RUN_TEST(test_a_loose_ball_with_nobody_near_it_is_not_one_to_throw_himself_on);
	RUN_TEST(test_a_team_mate_on_the_ball_next_to_him_is_left_alone);
	RUN_TEST(test_a_ball_he_may_not_handle_is_not_one_to_throw_himself_on);
	RUN_TEST(test_the_smother_control_throws_him_along_its_direction);
	RUN_TEST(test_landing_on_the_ball_puts_it_in_his_hands);
	RUN_TEST(test_an_opponent_he_hits_on_the_way_is_tackled);
	RUN_TEST(test_an_opponent_out_of_his_reach_stays_on_his_feet);
	RUN_TEST(test_a_ball_barred_to_his_hands_after_he_has_gone_for_it_is_still_caught);
	RUN_TEST(test_he_gets_up_from_the_ball_holding_it);
	RUN_TEST(test_missing_the_ball_puts_him_back_on_his_feet_in_goal);

	return UNITY_END();
}
