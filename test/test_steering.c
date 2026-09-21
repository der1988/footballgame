#include "entity.h"
#include "entity_player.h"
#include "game.h"
#include "log.h"
#include "steering.h"
#include "types.h"
#include "unity/unity.h"
#include "unity/unity_internals.h"
#include <raymath.h>

#define TOP_SPEED 110.0f

static struct entity player_entity;
static struct game_state game_state;

void setUp(void)
{
	player_entity = (struct entity){0};
	player_entity.player.constants.movement_force = 35000;
	player_entity.player.constants.mass = 77.5f;
	player_entity.player.constants.run_max_velocity = TOP_SPEED;

	game_state.constants.player_steering_brake_ratio = 0.6f;
	game_state.constants.player_steering_band = 10;
}

void tearDown(void) {}

static v2 player_arrive(v2 velocity, v2 target)
{
	player_entity.physics.velocity = velocity;
	return entity_player_steer_arrive(&player_entity, target, &game_state);
}

void test_player_arrive_from_rest_pushes_full_stick_at_a_far_target(void)
{
	v2 steering = player_arrive(Vector2Zero(), (v2){1000, 0});

	TEST_ASSERT_FLOAT_WITHIN(0.001f, 1, steering.x);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 0, steering.y);
}

// The velocity correction this replaced faded to nothing here, which is why AI players never
// matched a human's acceleration.
void test_player_arrive_at_top_speed_keeps_full_stick_while_the_target_is_far(void)
{
	v2 steering = player_arrive((v2){TOP_SPEED, 0}, (v2){1000, 0});

	TEST_ASSERT_FLOAT_WITHIN(0.001f, 1, steering.x);
}

void test_player_arrive_at_top_speed_brakes_near_the_target(void)
{
	v2 steering = player_arrive((v2){TOP_SPEED, 0}, (v2){5, 0});

	TEST_ASSERT_FLOAT_WITHIN(0.001f, -1, steering.x);
}

void test_player_arrive_cancels_sideways_drift_near_the_target(void)
{
	v2 steering = player_arrive((v2){0, TOP_SPEED}, (v2){5, 0});

	TEST_ASSERT_LESS_THAN_FLOAT(0, steering.y);
}

void test_player_arrive_on_the_target_at_rest_lets_go_of_the_stick(void)
{
	v2 steering = player_arrive(Vector2Zero(), Vector2Zero());

	TEST_ASSERT_FLOAT_WITHIN(0.001f, 0, Vector2Length(steering));
}

// The velocity correction reached twice the stick's length on a reversal
void test_player_arrive_never_pushes_harder_than_a_full_stick(void)
{
	v2 velocities[] = {{-TOP_SPEED, 0}, {0, TOP_SPEED}, {TOP_SPEED, TOP_SPEED}, {0, 0}};
	v2 targets[] = {{1000, 0}, {5, 0}, {-300, 40}, {0, 0}};

	for (usize v = 0; v < ARRAY_LEN(velocities); ++v) {
		for (usize t = 0; t < ARRAY_LEN(targets); ++t) {
			TEST_ASSERT_LESS_OR_EQUAL_FLOAT(
				1.0001f, Vector2Length(player_arrive(velocities[v], targets[t])));
		}
	}
}

// Running flat out at a far target, a limit of half his top speed has him braking down to it
void test_player_limited_arrive_brakes_down_to_its_limit(void)
{
	player_entity.physics.velocity = (v2){TOP_SPEED, 0};
	v2 steering = entity_player_steer_arrive_limited(
		&player_entity, (v2){1000, 0}, TOP_SPEED * 0.5f, &game_state);

	TEST_ASSERT_FLOAT_WITHIN(0.001f, -1, steering.x);
}

void test_player_limited_arrive_holds_its_limit(void)
{
	player_entity.physics.velocity = (v2){TOP_SPEED * 0.5f, 0};
	v2 steering = entity_player_steer_arrive_limited(
		&player_entity, (v2){1000, 0}, TOP_SPEED * 0.5f, &game_state);

	TEST_ASSERT_FLOAT_WITHIN(0.001f, 0, steering.x);
}

void test_player_seek_is_a_full_stick_at_the_target(void)
{
	player_entity.transform.position = (v2){10, 10};
	v2 steering = entity_player_steer_seek(&player_entity, (v2){10, 50});

	TEST_ASSERT_FLOAT_WITHIN(0.001f, 0, steering.x);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 1, steering.y);
}

void test_player_pursuit_leads_a_moving_target(void)
{
	v2 steering = entity_player_steer_pursuit(&player_entity, (v2){100, 0}, (v2){0, 50});

	TEST_ASSERT_GREATER_THAN_FLOAT(0, steering.y);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 1, Vector2Length(steering));
}

// The velocity correction in steering.c, kept for movers that are not players

void test_seek_from_rest_points_at_the_target(void)
{
	v2 steering = steering_seek(Vector2Zero(), Vector2Zero(), TOP_SPEED, (v2){0, 100});

	TEST_ASSERT_FLOAT_WITHIN(0.001f, 0, steering.x);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 1, steering.y);
}

// What the camera relies on to settle
void test_seek_fades_at_max_velocity(void)
{
	v2 steering = steering_seek(Vector2Zero(), (v2){TOP_SPEED, 0}, TOP_SPEED, (v2){1000, 0});

	TEST_ASSERT_FLOAT_WITHIN(0.001f, 0, Vector2Length(steering));
}

// Turning round at max velocity, the uncapped correction would be twice as long
void test_seek_stays_at_full_length_on_a_reversal(void)
{
	v2 steering = steering_seek(Vector2Zero(), (v2){-TOP_SPEED, 0}, TOP_SPEED, (v2){1000, 0});

	TEST_ASSERT_FLOAT_WITHIN(0.001f, 1, steering.x);
}

void test_arrive_stays_at_full_length_on_a_reversal(void)
{
	v2 steering = steering_arrive(Vector2Zero(), (v2){-TOP_SPEED, 0}, TOP_SPEED, (v2){1000, 0}, 20);

	TEST_ASSERT_FLOAT_WITHIN(0.001f, 1, steering.x);
}

void test_pursuit_stays_at_full_length_on_a_reversal(void)
{
	v2 steering = steering_pursuit(
		Vector2Zero(), (v2){-TOP_SPEED, 0}, TOP_SPEED, (v2){1000, 0}, Vector2Zero());

	TEST_ASSERT_FLOAT_WITHIN(0.001f, 1, steering.x);
}

void test_arrive_asks_for_less_speed_inside_the_slowing_distance(void)
{
	// Half way into the slowing distance, so it asks for half of max velocity
	v2 steering = steering_arrive(Vector2Zero(), Vector2Zero(), TOP_SPEED, (v2){10, 0}, 20);

	TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, steering.x);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, 0, steering.y);
}

void test_arrive_brakes_on_the_target(void)
{
	v2 steering =
		steering_arrive(Vector2Zero(), (v2){TOP_SPEED * 0.5f, 0}, TOP_SPEED, Vector2Zero(), 20);

	TEST_ASSERT_FLOAT_WITHIN(0.001f, -0.5f, steering.x);
}

void test_arrive_on_the_target_at_rest_is_zero(void)
{
	v2 steering = steering_arrive((v2){5, 5}, Vector2Zero(), TOP_SPEED, (v2){5, 5}, 20);

	TEST_ASSERT_FLOAT_WITHIN(0.001f, 0, Vector2Length(steering));
}

void test_pursuit_leads_a_moving_target(void)
{
	v2 steering =
		steering_pursuit(Vector2Zero(), Vector2Zero(), TOP_SPEED, (v2){100, 0}, (v2){0, 50});

	TEST_ASSERT_GREATER_THAN_FLOAT(0, steering.y);
}

void test_pursuit_of_a_still_target_is_a_seek(void)
{
	v2 velocity = {0, 30};
	v2 pursuit = steering_pursuit(Vector2Zero(), velocity, TOP_SPEED, (v2){100, 0}, Vector2Zero());
	v2 seek = steering_seek(Vector2Zero(), velocity, TOP_SPEED, (v2){100, 0});

	TEST_ASSERT_FLOAT_WITHIN(0.001f, seek.x, pursuit.x);
	TEST_ASSERT_FLOAT_WITHIN(0.001f, seek.y, pursuit.y);
}

int main()
{
	GLOG_INFO(LS_TESTS, "steering test - start");

	UNITY_BEGIN();

	RUN_TEST(test_player_arrive_from_rest_pushes_full_stick_at_a_far_target);
	RUN_TEST(test_player_arrive_at_top_speed_keeps_full_stick_while_the_target_is_far);
	RUN_TEST(test_player_arrive_at_top_speed_brakes_near_the_target);
	RUN_TEST(test_player_arrive_cancels_sideways_drift_near_the_target);
	RUN_TEST(test_player_arrive_on_the_target_at_rest_lets_go_of_the_stick);
	RUN_TEST(test_player_arrive_never_pushes_harder_than_a_full_stick);
	RUN_TEST(test_player_limited_arrive_brakes_down_to_its_limit);
	RUN_TEST(test_player_limited_arrive_holds_its_limit);
	RUN_TEST(test_player_seek_is_a_full_stick_at_the_target);
	RUN_TEST(test_player_pursuit_leads_a_moving_target);
	RUN_TEST(test_seek_from_rest_points_at_the_target);
	RUN_TEST(test_seek_fades_at_max_velocity);
	RUN_TEST(test_seek_stays_at_full_length_on_a_reversal);
	RUN_TEST(test_arrive_stays_at_full_length_on_a_reversal);
	RUN_TEST(test_pursuit_stays_at_full_length_on_a_reversal);
	RUN_TEST(test_arrive_asks_for_less_speed_inside_the_slowing_distance);
	RUN_TEST(test_arrive_brakes_on_the_target);
	RUN_TEST(test_arrive_on_the_target_at_rest_is_zero);
	RUN_TEST(test_pursuit_leads_a_moving_target);
	RUN_TEST(test_pursuit_of_a_still_target_is_a_seek);

	return UNITY_END();
}
