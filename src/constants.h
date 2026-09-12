#ifndef CONSTANTS_H
#define CONSTANTS_H

#define PIXELS_PER_METER			 (1080.0f / 108.0f)
#define GOALIE_DIVE_HEIGHT_THRESHOLD 12
#define GOALIE_LOOKAHEAD_MS			 0.3f
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
// He carries the ball slower than he runs without it
#define GOALIE_CARRY_SPEED_RATIO 0.75f
// And pulls up harder than a player running free, so the ball in his hands has some weight
#define GOALIE_CARRY_DRAG_RATIO 1.5f
// Height the ball sits at while it is in his hands
#define GOALIE_HOLD_BALL_HEIGHT		 12
#define BALL_MAX_LOOKAHEADS			 30
#define MAX_HUMAN_PLAYERS			 2
#define SHOOT_CHARGE_MULTIPLIER		 2
#define PASS_CHARGE_MULTIPLIER		 2
#define LONG_PASS_CHARGE_MULTIPLIER	 2
#define INPUT_STICK_DEAD_ZONE		 0.05f
#define BALL_PASS_MAX_TRAVEL_TIME	 1.5f
#define PASS_AIM_ITERATIONS			 6
// Relaxation factor applied after the first iteration. A receiver running back towards the
// passer makes the undamped iteration oscillate instead of settling: a shorter aim means a
// shorter travel time, which means less lead, which means a longer aim again.
#define PASS_AIM_RELAXATION 0.6f
// The turn is done, and the skid over, once the run points this close to the heading asked for
#define DRIBBLE_END_ALIGNMENT 0.7f
// Steering staying this close to the heading the skid started on is the same request being held,
// anything further off is a new one
#define DRIBBLE_SAME_REQUEST_ALIGNMENT 0.7f
// A player moving slower than this is standing still
#define PLAYER_AT_REST_SPEED 3.0f

// The scene is rendered one virtual pixel wider than the virtual screen on every side, so
// that scrolling the final blit by a sub-pixel camera offset never exposes an empty edge.
#define VIRTUAL_SCREEN_BLEED 1

#endif // !CONSTANTS_H
