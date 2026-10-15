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

static const enum team_formation selectable_formations[] = {TF_541, TF_532, TF_442, TF_343, TF_3232};

void test_selectable_formations_have_ten_valid_outfield_roles(void)
{
	const int expected_defenders[] = {5, 5, 4, 3, 3};
	const int expected_forwards[] = {1, 2, 2, 3, 2};
	for (usize f = 0; f < ARRAY_LEN(selectable_formations); ++f) {
		int defenders = 0, forwards = 0;
		for (int i = 0; i < 10; ++i) {
			struct team_formation_position *slot = football_tactics_get_team_formation_position(selectable_formations[f], i);
			TEST_ASSERT_NOT_NULL(slot->name);
			enum player_role role = slot->supported_roles[0];
			defenders += role == PR_CB || role == PR_SB || role == PR_WB;
			// In the 3-4-3 the wing-backs belong to the midfield line.
			if (selectable_formations[f] == TF_343 && role == PR_WB) --defenders;
			forwards += role == PR_CF || role == PR_WF;
			for (int phase = 0; phase < TM_COUNT; ++phase) {
				TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.5f, slot->position[phase].x);
				TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.5f, slot->position[phase].y);
			}
			TEST_ASSERT_LESS_THAN_FLOAT(slot->position[TM_DEFAULT].y, slot->position[TM_DEFENDING].y);
			TEST_ASSERT_GREATER_THAN_FLOAT(slot->position[TM_DEFAULT].y, slot->position[TM_ATTACKING].y);
		}
		TEST_ASSERT_EQUAL_INT(expected_defenders[f], defenders);
		TEST_ASSERT_EQUAL_INT(expected_forwards[f], forwards);
	}
}

void test_pre_match_codes_select_the_requested_formation(void)
{
	const char *codes[] = {"541", "532", "442", "343", "3232"};
	for (usize i = 0; i < ARRAY_LEN(codes); ++i)
		TEST_ASSERT_EQUAL_INT(selectable_formations[i], football_tactics_formation_from_code(codes[i]));
	TEST_ASSERT_EQUAL_INT(TF_442, football_tactics_formation_from_code("invalid"));
	TEST_ASSERT_EQUAL_INT(TF_442, football_tactics_formation_from_code(NULL));
}

void test_formation_lines_are_mirrored_toward_their_own_goal(void)
{
	struct game_state game = {0};
	struct entity player = {0}, ball = {0};
	game.vars.pitch.field = (rect_t){0, 0, 1000, 1000};
	game.gameplay.state_machine.current_state = FS_KICKOFF;
	game.vars.teams[TS_HOME].play_context.pitch_side = PSI_NORTH;
	game.vars.teams[TS_AWAY].play_context.pitch_side = PSI_SOUTH;
	for (usize f = 0; f < ARRAY_LEN(selectable_formations); ++f) {
		game.vars.teams[TS_HOME].play_context.formation = selectable_formations[f];
		game.vars.teams[TS_AWAY].play_context.formation = selectable_formations[f];
		for (int i = 0; i < 10; ++i) {
			player.player.team_formation_position_idx = i;
			player.player.team_side = TS_HOME;
			v2 north = football_tactics_get_target_position(&player, &ball, &game);
			player.player.team_side = TS_AWAY;
			v2 south = football_tactics_get_target_position(&player, &ball, &game);
			TEST_ASSERT_FLOAT_WITHIN(0.01f, north.x, south.x);
			TEST_ASSERT_FLOAT_WITHIN(0.01f, 1000, north.y + south.y);
			if (i == 0) TEST_ASSERT_LESS_THAN_FLOAT(south.y, north.y);
		}
	}
}

void test_tactical_identity_changes_height_width_and_pressing(void)
{
	struct game_state game = {0};
	struct entity entities[3] = {0};
	struct entity_repository repo = {.initialised = true, .entities = entities, .max_slots = 3};
	game.entity_repository = &repo;
	game.vars.pitch.field = (rect_t){0, 0, 1000, 1000};
	game.vars.teams[TS_HOME].play_context.pitch_side = PSI_NORTH;
	game.gameplay.state_machine.current_state = FS_PLAY;
	entities[0].transform.position = (v2){500, 700};
	entities[0].ball.last_touch_entity_id = 1;
	entities[1].player.team_side = TS_HOME;
	entities[2].player.team_side = TS_AWAY;
	f32 attacking_height[5] = {0}, defensive_height[5] = {0};
	for (usize f = 0; f < ARRAY_LEN(selectable_formations); ++f) {
		game.vars.teams[TS_HOME].play_context.formation = selectable_formations[f];
		for (int i = 0; i < 10; ++i) {
			entities[1].player.team_formation_position_idx = i;
			entities[0].ball.last_touch_entity_id = 1;
			attacking_height[f] += football_tactics_get_target_position(&entities[1], &entities[0], &game).y / 10;
			entities[0].ball.last_touch_entity_id = 2;
			defensive_height[f] += football_tactics_get_target_position(&entities[1], &entities[0], &game).y / 10;
		}
		TEST_ASSERT_GREATER_THAN_FLOAT(defensive_height[f], attacking_height[f]);
	}
	TEST_ASSERT_LESS_THAN_FLOAT(attacking_height[2], attacking_height[0]);
	TEST_ASSERT_GREATER_THAN_FLOAT(attacking_height[2], attacking_height[3]);
	TEST_ASSERT_LESS_THAN_FLOAT(defensive_height[2], defensive_height[0]);
	TEST_ASSERT_GREATER_THAN_FLOAT(defensive_height[2], defensive_height[3]);
	TEST_ASSERT_LESS_THAN_FLOAT(football_tactics_get_profile(TF_442)->pressing_distance, football_tactics_get_profile(TF_541)->pressing_distance);
	TEST_ASSERT_GREATER_THAN_FLOAT(football_tactics_get_profile(TF_442)->pressing_distance, football_tactics_get_profile(TF_343)->pressing_distance);
	TEST_ASSERT_GREATER_THAN_FLOAT(football_tactics_get_profile(TF_541)->defensive_width, football_tactics_get_profile(TF_442)->defensive_width);
	TEST_ASSERT_GREATER_THAN_FLOAT(football_tactics_get_profile(TF_3232)->attacking_width, football_tactics_get_profile(TF_343)->attacking_width);
}

void test_forward_checks_and_runs_without_abandoning_his_lane(void)
{
	struct game_state game = {0};
	struct entity entities[3] = {0};
	struct entity_repository repo = {.initialised = true, .entities = entities, .max_slots = 3};
	game.entity_repository = &repo;
	game.vars.pitch.field = (rect_t){0,0,1000,1000};
	game.vars.teams[TS_HOME].play_context.formation = TF_442;
	game.vars.teams[TS_HOME].play_context.pitch_side = PSI_NORTH;
	game.gameplay.state_machine.current_state = FS_PLAY;
	entities[0].transform.position = (v2){500,600};
	entities[0].ball.last_touch_entity_id = 1;
	entities[1].player.team_side = TS_HOME;
	entities[1].player.team_formation_position_idx = 8;
	v2 first = football_tactics_get_target_position(&entities[1], &entities[0], &game);
	game.time = 3;
	v2 next = football_tactics_get_target_position(&entities[1], &entities[0], &game);
	TEST_ASSERT_GREATER_THAN_FLOAT(20, Vector2Distance(first, next));
	TEST_ASSERT_FLOAT_WITHIN(100, 370, next.x);
	TEST_ASSERT_GREATER_THAN_FLOAT(700, next.y);
}

void test_restart_reserves_ball_for_correct_team_and_waits(void)
{
	struct game_state game = {0};
	struct entity entities[5] = {0};
	struct entity_repository repo = {.initialised = true, .entities = entities, .max_slots = 5};
	game.entity_repository = &repo;
	game.vars.ball.entity = &entities[0];
	game.vars.pitch.field = (rect_t){0,0,1000,1000};
	game.vars.pitch.midfield_spot = (v2){500,500};
	game.vars.teams[TS_HOME].play_context = (struct team_play_context){.formation=TF_442, .pitch_side=PSI_NORTH};
	game.vars.teams[TS_AWAY].play_context = (struct team_play_context){.formation=TF_442, .pitch_side=PSI_SOUTH};
	entities[0].transform.position = (v2){0,500};
	entities[0].ball.last_touch_entity_id = 1;
	for (int i=1;i<5;i++) {
		entities[i].header.id=i;
		entities[i].header.alive=true;
		entities[i].header.type=ET_PLAYER;
		entities[i].player.team_side=i<3 ? TS_HOME : TS_AWAY;
		entities[i].player.team_formation_position_idx=i;
		entities[i].transform.position=(v2){100*i,500};
	}
	game.gameplay.state_machine.current_state=FS_THROW_IN;
	game.time=10;
	gameplay_prepare_restart(&game);
	TEST_ASSERT_TRUE(game.gameplay.restart.prepared);
	TEST_ASSERT_EQUAL_INT(TS_AWAY, game.gameplay.restart.team);
	TEST_ASSERT_EQUAL_INT(TS_AWAY, entities[game.gameplay.restart.taker].player.team_side);
	TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(94, Vector2Distance(entities[1].transform.position, game.gameplay.restart.spot));
	game.time=11.9;
	gameplay_update(&game);
	TEST_ASSERT_EQUAL_INT(FS_THROW_IN, game.gameplay.state_machine.current_state);
	TEST_ASSERT_TRUE(game.gameplay.restart.prepared);
}

void test_foul_inside_area_awards_penalty_outside_awards_free_kick(void)
{
	for (int inside=0; inside<2; ++inside) {
		struct game_state game = {0};
		struct entity entities[3] = {0};
		struct entity_repository repo = {.initialised=true, .entities=entities, .max_slots=3};
		game.entity_repository=&repo;
		game.vars.ball.entity=&entities[0];
		game.vars.pitch.field=(rect_t){0,0,1000,1000};
		game.vars.pitch.goal[PSI_NORTH].penalty_box=(rect_t){300,0,400,180};
		game.vars.pitch.goal[PSI_NORTH].penalty_spot=(v2){500,120};
		game.vars.teams[TS_HOME].play_context=(struct team_play_context){.formation=TF_442,.pitch_side=PSI_NORTH};
		game.vars.teams[TS_AWAY].play_context=(struct team_play_context){.formation=TF_442,.pitch_side=PSI_SOUTH};
		entities[0].ball.last_touch_entity_id=2;
		for(int i=1;i<3;++i) {
			entities[i].header.id=i;
			entities[i].header.alive=true;
			entities[i].header.type=ET_PLAYER;
			entities[i].player.team_side=i==1 ? TS_HOME : TS_AWAY;
		}
		entities[2].transform.position=(v2){500,inside ? 100 : 350};
		game.gameplay.mode=GPM_MATCH;
		state_machine_football_init(&game.gameplay.state_machine, &game.gameplay, gameplay_match_vtable.transitions, FS_PLAY, &game);
		gameplay_award_foul(&entities[1], &entities[2], &game);
		TEST_ASSERT_EQUAL_INT(inside ? FS_PENALTY_KICK : FS_FREE_KICK, game.gameplay.state_machine.current_state);
		TEST_ASSERT_EQUAL_INT(TS_AWAY, game.gameplay.restart.team);
		TEST_ASSERT_TRUE(game.gameplay.restart.prepared);
		TEST_ASSERT_EQUAL_FLOAT(inside ? 120 : 350, game.gameplay.restart.spot.y);
	}
}

int main()
{
	GLOG_INFO(LS_TESTS, "gameplay test - start");

	UNITY_BEGIN();

	RUN_TEST(test_forward_checks_and_runs_without_abandoning_his_lane);
	RUN_TEST(test_restart_reserves_ball_for_correct_team_and_waits);
	RUN_TEST(test_foul_inside_area_awards_penalty_outside_awards_free_kick);
	RUN_TEST(test_cells_are_in_range);
	RUN_TEST(test_none_row_is_empty);
	RUN_TEST(test_no_state_reachable_from_play_is_terminal);
	RUN_TEST(test_every_trigger_is_mapped_by_some_mode);
	RUN_TEST(test_string_tables_are_complete);
	RUN_TEST(test_referee_follows_play_without_moving_the_ball);
	RUN_TEST(test_referee_stays_inside_pitch_when_ball_goes_out);
	RUN_TEST(test_referee_moves_clear_of_a_player_at_his_target);
	RUN_TEST(test_selectable_formations_have_ten_valid_outfield_roles);
	RUN_TEST(test_pre_match_codes_select_the_requested_formation);
	RUN_TEST(test_formation_lines_are_mirrored_toward_their_own_goal);
	RUN_TEST(test_tactical_identity_changes_height_width_and_pressing);

	return UNITY_END();
}
