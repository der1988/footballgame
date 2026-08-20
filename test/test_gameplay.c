#include "football.h"
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

int main()
{
	GLOG_INFO(LS_TESTS, "gameplay test - start");

	UNITY_BEGIN();

	RUN_TEST(test_cells_are_in_range);
	RUN_TEST(test_none_row_is_empty);
	RUN_TEST(test_no_state_reachable_from_play_is_terminal);
	RUN_TEST(test_every_trigger_is_mapped_by_some_mode);
	RUN_TEST(test_string_tables_are_complete);

	return UNITY_END();
}
