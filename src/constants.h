#ifndef CONSTANTS_H
#define CONSTANTS_H

#define PIXELS_PER_METER			 (1080.0f / 108.0f)
#define GOALIE_DIVE_HEIGHT_THRESHOLD 12
#define GOALIE_LOOKAHEAD_MS			 0.3f
#define BALL_MAX_LOOKAHEADS			 30
#define MAX_HUMAN_PLAYERS			 2
#define SHOOT_CHARGE_MULTIPLIER		 2
#define PASS_CHARGE_MULTIPLIER		 2
#define LONG_PASS_CHARGE_MULTIPLIER	 2
#define BALL_PASS_MAX_TRAVEL_TIME	 1.5f
#define PASS_AIM_ITERATIONS			 6
// Relaxation factor applied after the first iteration. A receiver running back towards the
// passer makes the undamped iteration oscillate instead of settling: a shorter aim means a
// shorter travel time, which means less lead, which means a longer aim again.
#define PASS_AIM_RELAXATION 0.6f

#endif // !CONSTANTS_H
