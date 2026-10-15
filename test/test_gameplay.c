#include "football.h"
#include "entity.h"
#include "game.h"
#include "system.h"
#include <raymath.h>
#include "gameplay.h"
#include "gameplay_match.h"
#include "gameplay_sandbox.h"
#include "log.h"
#include "types.h"
#include "unity/unity.h"
#include "unity/unity_internals.h"

void setUp(void) {}

void tearDown(void) {}

static const struct gameplay_vtable *vtables[] = {&gameplay_match_vtable, &gameplay_sandbox_vtable};

// An unmapped cell and a cell deliberately holding FS_NONE are the same bytes,
// so the tables can only be checked for what is actually distinguishable: no
// destination outside the enum, and no row on the never-fired FT_NONE column.
void test_cells_are_in_range(void)
{
	for (usize v = 0; v < ARRAY_LEN(vtables); ++v) {
		const state_machine_football_transition_row_t *transitions = vtables[v]->transitions;
		for (usize state = 0; state < FS_COUNT; ++state) {
			TEST_ASSERT_EQUAL_INT(FS_NONE, transitions[state][FT_NONE]);
			for (usize trigger = 0; trigger < FT_COUNT; ++trigger) {
				TEST_ASSERT_LESS_THAN_UINT(FS_COUNT, transitions[state][trigger]);
			}
		}
	}
}

void test_none_row_is_empty(void)
{
	for (usize v = 0; v < ARRAY_LEN(vtables); ++v) {
		const state_machine_football_transition_row_t *transitions = vtables[v]->transitions;
		for (usize trigger = 0; trigger < FT_COUNT; ++trigger) {
			TEST_ASSERT_EQUAL_INT(FS_NONE, transitions[FS_NONE][trigger]);
		}
	}
}

// A state play can reach but never leave hangs the session: fire() panics on
// every trigger the row does not map, so there is no way back out.
void test_no_state_reachable_from_play_is_terminal(void)
{
	for (usize v = 0; v < ARRAY_LEN(vtables); ++v) {
		const state_machine_football_transition_row_t *transitions = vtables[v]->transitions;

		b32 reachable[FS_COUNT] = {0};
		reachable[FS_PLAY] = true;
		for (b32 grew = true; grew;) {
			grew = false;
			for (usize state = 0; state < FS_COUNT; ++state) {
				if (!reachable[state]) {
					continue;
				}
				for (usize trigger = 0; trigger < FT_COUNT; ++trigger) {
					const enum state_machine_football_state to = transitions[state][trigger];
					if (to != FS_NONE && !reachable[to]) {
						reachable[to] = true;
						grew = true;
					}
				}
			}
		}

		for (usize state = 0; state < FS_COUNT; ++state) {
			if (!reachable[state]) {
				continue;
			}
			b32 has_exit = false;
			for (usize trigger = 0; trigger < FT_COUNT; ++trigger) {
				has_exit |= transitions[state][trigger] != FS_NONE;
			}
			TEST_ASSERT_TRUE_MESSAGE(has_exit, state_machine_football_state_string[state]);
		}
	}
}

// A trigger no table names is dead weight that reads as supported.
void test_every_trigger_is_mapped_by_some_mode(void)
{
	for (usize trigger = FT_NONE + 1; trigger < FT_COUNT; ++trigger) {
		b32 mapped = false;
		for (usize v = 0; v < ARRAY_LEN(vtables); ++v) {
			const state_machine_football_transition_row_t *transitions = vtables[v]->transitions;
			for (usize state = 0; state < FS_COUNT; ++state) {
				mapped |= transitions[state][trigger] != FS_NONE;
			}
		}
		TEST_ASSERT_TRUE_MESSAGE(mapped, state_machine_football_trigger_string[trigger]);
	}
}

// A missing entry turns fire()'s panic path into a null dereference, losing the
// diagnostic exactly when it is needed.
void test_string_tables_are_complete(void)
{
	for (usize state = 0; state < FS_COUNT; ++state) {
		TEST_ASSERT_NOT_NULL(state_machine_football_state_string[state]);
	}
	for (usize trigger = 0; trigger < FT_COUNT; ++trigger) {
		TEST_ASSERT_NOT_NULL(state_machine_football_trigger_string[trigger]);
	}
}

static void setup_referee_scene(struct game_state *game, struct entity_repository *repo,
							   struct entity entities[3])
{
	repo->entities = entities;
	repo->max_slots = 3;
	game->entity_repository = repo;
	game->dt = 1.0 / 60;
	game->vars.pitch.field = (rect_t){0, 0, 1000, 800};
	game->vars.ball.entity = &entities[0];
	entities[0].header.alive = true;
	entities[0].header.type = ET_BALL;
	entities[0].transform.position = (v2){800, 300};
	entities[1].header.alive = true;
	entities[1].header.type = ET_REFEREE;
	entities[1].transform.header.enabled = true;
	entities[1].physics.header.enabled = true;
	entities[1].physics.mass = 1;
	entities[1].physics.max_velocity = 125;
	entities[1].sprite.header.enabled = true;
	entities[1].transform.position = (v2){200, 500};
	for (usize i = 0; i < SO_COUNT; ++i) {
		game->gfx.referee.stand_frames[i] = 100 + i;
		for (usize j = 0; j < 4; ++j) game->gfx.referee.run_frames[i][j] = 200 + i * 4 + j;
	}
}

void test_referee_follows_play_without_moving_the_ball(void)
{
	struct game_state game = {0};
	struct entity_repository repo = {0};
	struct entity entities[3] = {0};
	setup_referee_scene(&game, &repo, entities);
	for (int i = 0; i < 120; ++i) {
		game.time += game.dt;
		system_referee(&game);
		TEST_ASSERT_LESS_OR_EQUAL_FLOAT(125.01f, Vector2Length(entities[1].physics.velocity));
	}
	TEST_ASSERT_GREATER_THAN_FLOAT(350, entities[1].transform.position.x);
	TEST_ASSERT_LESS_THAN_FLOAT(450, entities[1].transform.position.y);
	TEST_ASSERT_EQUAL_FLOAT(800, entities[0].transform.position.x);
	TEST_ASSERT_EQUAL_FLOAT(300, entities[0].transform.position.y);
	TEST_ASSERT_GREATER_OR_EQUAL_UINT32(200, entities[1].sprite.frame_id);
	TEST_ASSERT_FALSE(entities[1].player.header.enabled);
}

void test_referee_stays_inside_pitch_when_ball_goes_out(void)
{
	struct game_state game = {0};
	struct entity_repository repo = {0};
	struct entity entities[3] = {0};
	setup_referee_scene(&game, &repo, entities);
	entities[0].transform.position = (v2){-200, -200};
	for (int i = 0; i < 600; ++i) {
		game.time += game.dt;
		system_referee(&game);
		TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(16, entities[1].transform.position.x);
		TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(16, entities[1].transform.position.y);
	}
	TEST_ASSERT_LESS_THAN_FLOAT(26, entities[1].transform.position.x);
	TEST_ASSERT_LESS_THAN_FLOAT(26, entities[1].transform.position.y);
	TEST_ASSERT_LESS_THAN_UINT32(200, entities[1].sprite.frame_id);
}

void test_referee_moves_clear_of_a_player_at_his_target(void)
{
	struct game_state game = {0};
	struct entity_repository repo = {0};
	struct entity entities[3] = {0};
	setup_referee_scene(&game, &repo, entities);
	entities[1].transform.position = (v2){720, 365};
	entities[2].header.alive = true;
	entities[2].header.type = ET_PLAYER;
	entities[2].transform.position = entities[1].transform.position;
	for (int i = 0; i < 30; ++i) system_referee(&game);
	TEST_ASSERT_LESS_THAN_FLOAT(710, entities[1].transform.position.x);
	TEST_ASSERT_EQUAL_FLOAT(720, entities[2].transform.position.x);
}

int main()
{
	GLOG_INFO(LS_TESTS, "gameplay test - start");

	UNITY_BEGIN();

	RUN_TEST(test_cells_are_in_range);
	RUN_TEST(test_none_row_is_empty);
	RUN_TEST(test_no_state_reachable_from_play_is_terminal);
	RUN_TEST(test_every_trigger_is_mapped_by_some_mode);
	RUN_TEST(test_string_tables_are_complete);
	RUN_TEST(test_referee_follows_play_without_moving_the_ball);
	RUN_TEST(test_referee_stays_inside_pitch_when_ball_goes_out);
	RUN_TEST(test_referee_moves_clear_of_a_player_at_his_target);

	return UNITY_END();
}
