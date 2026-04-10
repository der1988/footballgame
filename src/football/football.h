#ifndef FOOTBALL_H
#define FOOTBALL_H

#include "macro.h"
#include "types.h"

// Forward declaration
struct game_state;

enum match_state {
	MS_KICKOFF,
	MS_HALF_TIME,
	MS_EXTRA_TIME,
	MS_PENALTIES,
	MS_PLAY,
	MS_GOAL,
	MS_THROW_IN,
	MS_GOAL_KICK,
	MS_CORNER_KICK,
	MS_PENALTY_KICK,
	MS_FREE_KICK,
	MS_COUNT
};

extern const char *match_state_to_string[MS_COUNT];

struct match {
	enum match_state state;
	struct {
		u16 home;
		u16 away;
	} score;
};

enum pitch_type {
	PT_NORMAL,
	PT_MUDDY,
	PT_SNOWY
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
	enum pitch_type type;
};

void football_match_change_state(struct game_state *game_state, enum match_state new_state);

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

void football_setup(struct game_state *game_state);

#endif // !FOOTBALL_H
