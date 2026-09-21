#ifndef FOOTBALL_H
#define FOOTBALL_H

#include "events_router.h"
#include "macro.h"
#include "state_machine.h"
#include "types.h"

// Forward declaration
struct game_state;
struct entity;
struct gameplay;

enum state_machine_football_state {
	FS_NONE,
	FS_KICKOFF,
	FS_HALF_TIME,
	FS_EXTRA_TIME,
	FS_PENALTIES,
	FS_PLAY,
	FS_GOAL,
	FS_THROW_IN,
	FS_GOAL_KICK,
	FS_CORNER_KICK,
	FS_PENALTY_KICK,
	FS_FREE_KICK,
	FS_COUNT
};

extern const char *state_machine_football_state_string[FS_COUNT];

// The football situation is never the actor, so its triggers read as things
// that happened. A restart trigger names the restart, not the edge the ball
// crossed: the table is a pure lookup with nowhere to branch, so "corner or
// goal kick depending on last touch" has to be resolved before firing.
enum state_machine_football_trigger {
	FT_NONE,
	FT_GOAL_SCORED,
	FT_BALL_OUT_FOR_THROW_IN,
	FT_BALL_OUT_FOR_CORNER,
	FT_BALL_OUT_FOR_GOAL_KICK,
	FT_CELEBRATION_ENDED,
	FT_PLAY_RESUMED,
	FT_COUNT
};

extern const char *state_machine_football_trigger_string[FT_COUNT];

union state_machine_football_state_data {
	struct state_machine_default_empty_data kickoff;
	struct state_machine_default_empty_data half_time;
	struct state_machine_default_empty_data extra_time;
	struct state_machine_default_empty_data penalties;
	struct state_machine_default_empty_data play;
	struct {
		fg_time_t start_time;
	} goal;
	struct state_machine_default_empty_data throw_in;
	struct state_machine_default_empty_data goal_kick;
	struct state_machine_default_empty_data corner_kick;
	struct state_machine_default_empty_data penalty_kick;
	struct state_machine_default_empty_data free_kick;
};

STATE_MACHINE_DEFINE(football, struct gameplay, FT_COUNT);
STATE_MACHINE_DECLARE_FUNCTIONS(football, struct gameplay);

struct match {
	struct {
		u16 home;
		u16 away;
	} score;
	f32 celebration_time;
};

enum pitch_condition {
	PC_NORMAL,
	PC_MUDDY,
	PC_SNOWY
};

enum pitch_side {
	PSI_NORTH,
	PSI_SOUTH,
	PSI_COUNT
};

struct goal {
	struct entity *entity;
	v3 goal_quad_points[4];
	v2 center;
	rect_t penalty_box;
	v2 penalty_spot;
};

struct pitch {
	rect_t field;
	v2 midfield_spot;
	u16 goal_width;
	u16 goal_height;
	u16 penalty_box_width;
	u16 penalty_box_height;
	struct goal goal[PSI_COUNT];
	enum pitch_condition condition;
};

// East and west are always a throw-in; a byline exit is a corner or a goal kick
// depending on which team touched the ball last.
enum state_machine_football_trigger football_classify_ball_out(enum event_type event,
															   struct game_state *game_state);

enum player_role {
	PR_GK,	// Goalkeeper
	PR_SB,	// Side Back
	PR_CB,	// Centre Back
	PR_DMF, // Defensive Midfielder
	PR_CMF, // Central Midfielder
	PR_WB,	// Wing Back
	PR_SMF, // Side Midfielder
	PR_AMF, // Attacking Midfielder
	PR_WF,	// Wing Forward
	PR_SS,	// Second Striker
	PR_CF,	// Centre Forward
	PR_COUNT
};

enum team_formation {
	TF_433,
	TF_442,
	TF_352,
	TF_343,
	TF_532,
	TF_4312,
	TF_4231,
	TF_COUNT
};

enum style_of_play {
	SOP_BALANCED,
	SOP_OFFENSIVE,
	SOP_DEFENSIVE,
	SOP_COUNT
};

enum team_momentum {
	TM_DEFENDING,
	TM_DEFAULT,
	TM_ATTACKING,
	TM_COUNT
};

struct team_formation_position {
	const char *name;
	enum player_role supported_roles[3];
	v2 position[TM_COUNT];
	enum style_of_play style_of_play;
};

extern struct team_formation_position team_formations[TF_COUNT][10];

struct team_info {
	char *name;
};

enum team_side {
	TS_HOME,
	TS_AWAY,
	TS_COUNT
};

enum save_direction {
	SD_LEFT,
	SD_RIGHT,
	SD_COUNT
};

enum save_height {
	SH_HIGH,
	SH_LOW,
	SH_COUNT
};

struct goalie_dive {
	enum save_direction direction;
	enum save_height height;
	f32 speed; // 0-1, a fraction of goalie_dive_max_velocity
	f32 distance;
};

extern const char *pitch_side_to_string[PSI_COUNT];
extern const char *team_side_to_string[TS_COUNT];
extern const char *save_direction_to_string[SD_COUNT];
extern const char *save_height_to_string[SH_COUNT];

enum save_type {
	ST_WITHOUT_BALL,
	ST_WITH_BALL,
	ST_COUNT
};

struct team_formation_position *football_tactics_get_team_formation(enum team_formation formation);

struct team_formation_position *
football_tactics_get_team_formation_position(enum team_formation formation, u32 position_idx);
v2 football_tactics_get_target_position(struct entity *player_entity,
										struct entity *ball_entity,
										struct game_state *game_state);

b32 football_pitch_is_position_inside_penalty_area(v2 pos,
												   enum pitch_side side,
												   struct game_state *game_state);
b32 football_pitch_is_position_inside(v2 pos, struct game_state *game_state);

INLINE v2 football_pitch_position_normalized_to_abs(rect_t pitch, v2 pos)
{
	pos.x = pitch.x + pitch.width * pos.x;
	pos.y = pitch.y + pitch.height * (1 - pos.y);
	return pos;
}

INLINE v2 football_pitch_position_abs_to_normalized(rect_t pitch, v2 pos)
{
	pos.x = (pos.x - pitch.x) / pitch.width;
	pos.y = (pos.y - pitch.y) / pitch.height;
	return pos;
}

void football_pitch_setup(struct game_state *game_state);

#endif // !FOOTBALL_H
