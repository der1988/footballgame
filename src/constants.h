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
#define INPUT_STICK_DEAD_ZONE		 0.05f
#define BALL_PASS_MAX_TRAVEL_TIME	 1.5f
#define PASS_AIM_ITERATIONS			 6
// A short pass meant to go less far than this is rolled along the ground. Past it the lift grows
// with the distance, up to ball_pass_height at ball_pass_auto_max_distance.
#define PASS_LIFT_MIN_DISTANCE (8.0f * PIXELS_PER_METER)
// TODO: placeholder for the technique of every player, 0 to 1. Make it a player characteristic.
#define PASS_TECHNIQUE 0.5f
// Share of the lift that the best technique still can't take off a pass
#define PASS_LIFT_FLOOR 0.25f
// Relaxation factor applied after the first iteration. A receiver running back towards the
// passer makes the undamped iteration oscillate instead of settling: a shorter aim means a
// shorter travel time, which means less lead, which means a longer aim again.
#define PASS_AIM_RELAXATION 0.6f
// An automatic pass aims at least this far inside the lines, so a ball that rolls on past the aim
// point does not go straight out
#define PASS_AIM_FIELD_MARGIN (2.0f * PIXELS_PER_METER)
// An opponent this close to the line of a pass is in the way of it
#define PASS_INTERCEPTION_RADIUS 20.0f
// A human's automatic pass looks for a team mate this many degrees either side of the stick
#define PASS_SCAN_HALF_ANGLE 30.0f
// Share of ball_pass_force a short pass gets with no charge; the charge scales up the rest
#define PASS_MIN_FORCE_RATIO	  0.3f
// The turn is done, and the skid over, once the run points this close to the heading asked for
#define DRIBBLE_END_ALIGNMENT 0.7f
// Steering staying this close to the heading the skid started on is the same request being held,
// anything further off is a new one
#define DRIBBLE_SAME_REQUEST_ALIGNMENT 0.7f
// A player moving slower than this is standing still
#define PLAYER_AT_REST_SPEED 3.0f

// How long the keeper looks up before doing anything with the ball in his hands
#define GOALIE_AI_THINK_TIME 0.6f
// An opponent nearer than this and he just clears it
#define GOALIE_AI_PRESSURE_RADIUS 60.0f
// Nobody nearer than this and he can safely put it down and play out
#define GOALIE_AI_PLAY_OUT_RADIUS 200.0f
// How far he rolls the ball out by hand
#define GOALIE_AI_THROW_RANGE 250.0f
// How wide he looks for the man, measured off the up-the-pitch axis to either side, so the cone he
// scans is twice this
#define GOALIE_AI_THROW_HALF_ANGLE 80.0f
// Height the ball leaves his hands at when he rolls it out
#define GOALIE_THROW_HEIGHT 6
// How long he holds the follow-through pose once the ball has gone
#define GOALIE_THROW_FOLLOW_THROUGH_TIME 0.5f
// He carries the ball slower than he runs without it
#define GOALIE_CARRY_SPEED_RATIO 0.75f
// And pulls up harder than a player running free, so the ball in his hands has some weight
#define GOALIE_CARRY_DRAG_RATIO 1.5f
// Height the ball sits at while it is in his hands
#define GOALIE_HOLD_BALL_HEIGHT 12

// A player standing this close to his tactical spot stays put instead of chasing small shifts of it
#define PLAYER_AI_STAY_THERE_RADIUS 10.0f
// Per tick, at the fixed 480Hz step, so about a fifth of a second of carrying it on average
// once he has decided the ball should move.
#define PLAYER_AI_PASS_PROBABILITY_PER_TICK 0.01f
// The man who last played the ball loses this much of his AI pass score right after he played it,
// down to nothing once AI_PASS_BACK_TIME has gone by
#define AI_PASS_BACK_PENALTY 0.5f
#define AI_PASS_BACK_TIME	 3.0f

// Chance that a player hit by a tackle falls (PS_FALL) instead of starting a tackled run
// (PS_TACKLED_RUN).
#define TACKLE_KNOCKDOWN_PROBABILITY		 0.05f
#define SLIDING_TACKLE_KNOCKDOWN_PROBABILITY 0.3f
#define GOALIE_SMOTHER_KNOCKDOWN_PROBABILITY 0.3f
// How far past the edge of his control circle the ball can be and still be playable by the player
// in possession
#define PLAYING_DISTANCE 7.0f
// After a tackle plays the ball, the man it took it off can't gain possession for this long
#define TACKLE_DISPOSSESSION_BALL_LOCK_TIME 0.5f

// Height the ball is dropped from when play restarts
#define BALL_RESTART_HEIGHT		   30
#define BALL_IN_NET_DAMPING		   0.8f
#define BALL_IN_NET_RISING_DAMPING 0.9f
// Distance a rolling ball covers in one cycle of its roll animation
#define BALL_ROLL_DISTANCE_PER_CYCLE 8.0f
// How long the roll animation takes to catch up with a change of the ball's velocity, so a
// ball skids after a kick before it rolls
#define BALL_SPIN_CATCH_UP_TIME 0.2f
// Cycles per second. Four frames shown at 60Hz can't show a faster spin than this
#define BALL_MAX_SPIN 15.0f
// The ball is drawn rolling up to this height off the ground: dribbling touches lift it a little
#define BALL_ROLL_MAX_HEIGHT 3.0f
// A flying ball shows a random frame, changed at an interval picked when it is played: the slow
// interval for a ball that is barely moving, the fast one at the fast speed and above
#define BALL_AIR_FRAME_SLOW_INTERVAL 0.12f
#define BALL_AIR_FRAME_FAST_INTERVAL 0.04f
#define BALL_AIR_FRAME_FAST_SPEED	 (40.0f * PIXELS_PER_METER)
// The picked interval is off by up to this share either way, so no two flights look the same
#define BALL_AIR_FRAME_INTERVAL_JITTER 0.25f

#define GOAL_POST_WIDTH 2
// How far off a squarely struck post the deflection can be rotated, in degrees.
#define GOAL_DEFLECTION_ANGLE 120

#define MATCH_OUTFIELD_PLAYERS_PER_TEAM	  10
#define MATCH_CELEBRATION_TIME			  3.0f
#define SANDBOX_OUTFIELD_PLAYERS_PER_TEAM 0
// Long enough to see the ball settle in the net, short enough not to slow down
// repeated shots.
#define SANDBOX_RESTART_DELAY 1.0f

// The scene is rendered one virtual pixel wider than the virtual screen on every side, so
// that scrolling the final blit by a sub-pixel camera offset never exposes an empty edge.
#define VIRTUAL_SCREEN_BLEED 1

#endif // !CONSTANTS_H
