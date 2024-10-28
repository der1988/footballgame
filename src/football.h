#ifndef FOOTBALL
#define FOOTBALL

#include "macro.h"
#include "types.h"
#include <raylib.h>

// Forward declaration
struct game_state;

enum match_state {
    MS_KICKOFF,
    MS_PLAY,
    MS_GOAL,
    MS_THROW_IN,
    MS_GOAL_KICK,
    MS_CORNER_KICK,
    MS_PENALTY_KICK,
    MS_FREE_KICK,
    MS_COUNT
};

static const char *match_state_to_string[MS_COUNT] = {
    "kickoff", "play", "goal", "throw_in", "goal_kick", "corner_kick", "penalty_kick", "free_kick"};

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
    entity_id_t id;
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
    PR_GK,  // Goalkeeper
    PR_SB,  // Side Back
    PR_CB,  // Centre Back
    PR_DMF, // Defensive Midfielder
    PR_CMF, // Central Midfielder
    PR_WB,  // Wing Back
    PR_SMF, // Side Midfielder
    PR_AMF, // Attacking Midfielder
    PR_WF,  // Wing Forward
    PR_SS,  // Second Striker
    PR_CF,  // Centre Forward
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

static struct team_formation_position TeamFormations[TF_COUNT][10] = {
    // TF_433
    {
	{
	    .name = "LB",
	    .style_of_play = SOP_BALANCED,
	    .supported_roles = {PR_SB},
	    .position =
		{
		    {.x = 0.2f, .y = 0.1f},
		    {.x = 0.1f, .y = 0.3f},
		    {.x = 0.3f, .y = 0.65f},
		},
	},
	{
	    .name = "LB",
	    .style_of_play = SOP_BALANCED,
	    .supported_roles = {PR_SB},
	    .position =
		{
		    {.x = 0.8f, .y = 0.1f},
		    {.x = 0.9f, .y = 0.3f},
		    {.x = 0.7f, .y = 0.65f},
		},
	},
	{
	    .name = "CB",
	    .style_of_play = SOP_DEFENSIVE,
	    .supported_roles = {PR_CB},
	    .position =
		{
		    {.x = 0.4f, .y = 0.05f},
		    {.x = 0.4f, .y = 0.2f},
		    {.x = 0.4f, .y = 0.55f},
		},
	},
	{
	    .name = "CB",
	    .style_of_play = SOP_DEFENSIVE,
	    .supported_roles = {PR_CB},
	    .position =
		{
		    {.x = 0.6f, .y = 0.05f},
		    {.x = 0.6f, .y = 0.2f},
		    {.x = 0.6f, .y = 0.55f},
		},
	},
	{
	    .name = "DMF",
	    .style_of_play = SOP_BALANCED,
	    .supported_roles = {PR_DMF},
	    .position =
		{
		    {.x = 0.5f, .y = 0.15f},
		    {.x = 0.5f, .y = 0.4f},
		    {.x = 0.5f, .y = 0.75f},
		},
	},
	{
	    .name = "CMF",
	    .style_of_play = SOP_BALANCED,
	    .supported_roles = {PR_CMF},
	    .position =
		{
		    {.x = 0.35f, .y = 0.25f},
		    {.x = 0.35f, .y = 0.5f},
		    {.x = 0.35f, .y = 0.85f},
		},
	},
	{
	    .name = "CMF",
	    .style_of_play = SOP_BALANCED,
	    .supported_roles = {PR_CMF},
	    .position = {{.x = 0.65f, .y = 0.25f},
			 {.x = 0.65f, .y = 0.5f},
			 {.x = 0.65f, .y = 0.85f}},
	},
	{
	    .name = "LWF",
	    .style_of_play = SOP_OFFENSIVE,
	    .supported_roles = {PR_WF},
	    .position =
		{
		    {.x = 0.15f, .y = 0.35f},
		    {.x = 0.15f, .y = 0.6f},
		    {.x = 0.2f, .y = 0.92f},
		},
	},
	{
	    .name = "RWF",
	    .style_of_play = SOP_OFFENSIVE,
	    .supported_roles = {PR_WF},
	    .position =
		{
		    {.x = 0.85f, .y = 0.35f},
		    {.x = 0.85f, .y = 0.6f},
		    {.x = 0.8f, .y = 0.92f},
		},
	},
	{
	    .name = "CF",
	    .style_of_play = SOP_BALANCED,
	    .supported_roles = {PR_CF},
	    .position =
		{
		    {.x = 0.5f, .y = 0.45f},
		    {.x = 0.5f, .y = 0.7f},
		    {.x = 0.5f, .y = 0.95f},
		},
	},
    }};

struct team {
    char *name;
    enum team_formation formation;
};

struct team_formation_position *football_tactics_get_team_formation(enum team_formation formation);

struct team_formation_position *
football_tactics_get_team_formation_position(enum team_formation formation, u32 position_idx);

b32 football_pitch_is_position_inside_penalty_area(v2 pos,
						   enum pitch_side side,
						   struct game_state *game_state);
b32 football_pitch_is_position_inside(v2 pos, struct game_state *game_state);

INLINE v2 football_pitch_position_rel_to_abs(rect_t pitch, v2 pos)
{
    pos.x = pitch.x + pitch.width * pos.x;
    pos.y = pitch.y + pitch.height * (1 - pos.y);
    return pos;
}

INLINE v2 football_pitch_position_abs_to_rel(rect_t pitch, v2 pos)
{
    pos.x = (pos.x - pitch.x) / pitch.width;
    pos.y = (pos.y - pitch.y) / pitch.height;
    return pos;
}

#endif // !FOOTBALL
