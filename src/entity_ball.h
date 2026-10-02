#ifndef ENTITY_BALL_H
#define ENTITY_BALL_H

#include "football.h"
#include "state_machine.h"
#include "types.h"

#define DECLARE_BALL_ENTITY_COMPONENTS(entity)                                                     \
	DECLARE_ENTITY_COMPONENT_EXT(entity, transform, ball_transform);                               \
	DECLARE_ENTITY_COMPONENT_EXT(entity, physics3d, ball_physics_3d);                              \
	DECLARE_ENTITY_COMPONENT_EXT(entity, collider, ball_collider);                                 \
	DECLARE_ENTITY_COMPONENT_EXT(entity, ball, ball)

// Forward declarations
struct entity;
struct game_state;
struct component_ball;

struct ball_config {
	v2 position;
};

enum ball_tackle_type {
	BTT_STANDING,
	BTT_SLIDING,
};

// How the ball was last played on purpose. A standing tackle wins the ball rather than playing it,
// so it leaves the last_play_* fields alone.
enum ball_play_type {
	BPT_NONE,
	BPT_PASS,
	BPT_LONG_PASS,
	BPT_SHOT,
	BPT_HEADER,
	BPT_SLIDING_TACKLE,
	BPT_COUNT
};

extern const char *ball_play_type_string[BPT_COUNT];

enum ball_possession_type {
	BPOS_NONE,
	// His, with the ball within playing distance
	BPOS_PLAYABLE,
	// His by the timing rules, but knocked on beyond playing distance
	BPOS_OUT_OF_REACH,
	// The keeper is holding it
	BPOS_IN_HANDS,
	BPOS_COUNT
};

extern const char *ball_possession_type_string[BPOS_COUNT];

struct ball_possession {
	// EID_NOT_VALID for BPOS_NONE
	entity_id_t entity_id;
	enum ball_possession_type type;
};

// Which goalie may not use his hands on the ball
enum ball_not_pickable_by {
	BNPB_NOBODY,
	BNPB_HOME_GOALIE,
	BNPB_AWAY_GOALIE,
	BNPB_COUNT
};

extern const char *ball_not_pickable_by_string[BNPB_COUNT];

enum state_machine_ball_state {
	BS_NONE,
	BS_IN_PLAY,
	BS_OUT_OF_PLAY,
	BS_GK_HANDS,
	BS_COUNT
};

extern const char *state_machine_ball_state_string[BS_COUNT];

// The ball is never the actor, so its triggers read as things done to it.
enum state_machine_ball_trigger {
	BT_NONE,
	BT_CAUGHT,
	BT_RELEASED,
	BT_COUNT
};

extern const char *state_machine_ball_trigger_string[BT_COUNT];

union state_machine_ball_state_data {
	struct state_machine_default_empty_data in_play;
	struct state_machine_default_empty_data out_of_play;
	struct state_machine_default_empty_data gk_hands;
};

STATE_MACHINE_DEFINE(ball, struct entity, BT_COUNT);
STATE_MACHINE_DECLARE_FUNCTIONS(ball, struct entity);

struct entity *entity_ball_create(struct ball_config config, struct game_state *game_state);
void entity_ball_set_position_and_velocity(struct entity *ball_entity, v3 pos, v3 velocity);
void entity_ball_settle_in_net(struct entity *ball_entity,
							   rect_t goal_rectangle,
							   enum pitch_side side);
void entity_ball_control_precision(struct entity *ball_entity,
								   v2 player_velocity,
								   v2 position_to_seek,
								   f32 difficulty,
								   struct game_state *game_state);
void entity_ball_control_running(struct entity *ball_entity,
								 v2 control_direction,
								 v2 player_velocity,
								 f32 difficulty,
								 struct game_state *game_state);
void entity_ball_shoot(struct entity *ball_entity,
					   entity_id_t kicker_entity_id,
					   v2 direction,
					   f32 height,
					   f32 power,
					   b32 chip,
					   struct game_state *game_state);
void entity_ball_short_pass(struct entity *ball_entity,
							entity_id_t kicker_entity_id,
							v3 direction,
							f32 power,
							struct game_state *game_state);
void entity_ball_long_pass(struct entity *ball_entity,
						   entity_id_t kicker_entity_id,
						   v2 direction,
						   f32 power,
						   b32 driven,
						   struct game_state *game_state);
void entity_ball_tackle(struct entity *ball_entity,
						entity_id_t tackling_entity_id,
						v3 direction,
						f32 power,
						f32 force,
						enum ball_tackle_type type,
						struct game_state *game_state);
void entity_ball_header(struct entity *ball_entity,
						entity_id_t header_entity_id,
						v3 direction,
						f32 power,
						struct game_state *game_state);
void entity_ball_deviate_trajectory(struct entity *ball_entity, struct game_state *game_state);
void entity_ball_held_by_goalie(struct entity *ball_entity,
								entity_id_t goalie_entity_id,
								struct game_state *game_state);
void entity_ball_saved_by_goalie(struct entity *ball_entity,
								 entity_id_t goalie_entity_id,
								 struct game_state *game_state);
void entity_ball_released_by_goalie(struct entity *ball_entity,
									entity_id_t goalie_entity_id,
									struct game_state *game_state);
void entity_ball_in_goal(struct entity *ball_entity);
void entity_ball_set_last_touch(struct entity *ball_entity,
								entity_id_t player_on_the_ball_entity_id,
								struct game_state *game_state);
// Whether the goalie of this team may no longer use his hands on the ball: his own team played it
// with the feet or he released it, and the other team has not touched it nor a team mate headed it.
b32 entity_ball_is_goalie_hands_barred(struct entity *ball_entity, enum team_side team_side);
// Ends his possession, if the ball was last touched by him
void entity_ball_end_possession(struct entity *ball_entity, entity_id_t player_entity_id);
struct ball_possession entity_ball_player_in_possession(struct entity *ball_entity,
														struct game_state *game_state);
entity_id_t entity_ball_player_on_the_ball(struct entity *ball_entity,
										   struct game_state *game_state);
enum team_side entity_ball_team_in_possession(struct entity *ball_entity,
											  struct game_state *game_state);
v3 entity_ball_after_touch_acceleration(struct component_ball *ball,
										v2 direction,
										v3 ball_velocity,
										fg_time_t time,
										struct game_state *game_state);
fg_time_t entity_ball_time_to_cover_distance(struct entity *ball_entity,
											 v3 kick_velocity,
											 f32 distance,
											 struct game_state *game_state);

#endif // !ENTITY_BALL_H
