#ifndef ENTITY_PLAYER_H
#define ENTITY_PLAYER_H

#include "football.h"
#include "render.h"
#include "state_machine.h"
#include "types.h"
#include "as_animation.h"

// Forward declarations
struct entity;
struct list_entity_ids;
enum sprite_orientation;

enum state_machine_player_state {
	PS_NONE,
	PS_NORMAL,
	PS_THROW_IN,
	PS_HEADER,
	PS_SLIDING_TACKLE,
	PS_FALL,
	PS_GOALIE,
	PS_COUNT
};

extern const char *state_machine_player_state_string[PS_COUNT];

struct state_machine_player_normal_data {
	enum sprite_orientation running_action_orientation;
};

struct state_machine_running_action_state {
	running_action_idx_t running_action_idx;
};

struct state_machine_player_sliding_tackle_data {
	fg_time_t start_time;
	b32 touched_ball;
	b32 touched_player;
};

struct state_machine_player_fall_data {
	fg_time_t animation_start_time;
	fg_time_t animation_end_time;
	b32 animation_completed;
};

enum header_phase {
	HP_GROUND,
	HP_AIR_1,
	HP_AIR_2,
	HP_AIR_3,
	HP_MAX_ELEVATION,
	HP_COUNT
};

enum header_type {
	HEADER_TYPE_SHOOT,
	HEADER_TYPE_PASS,
};

struct state_machine_player_header_data {
	fg_time_t animation_start_time;
	b32 animation_completed;
	b32 elevation_completed;
	fg_time_t elevation_completion_time;
	b32 should_fall_down;
	u16 last_frame_id;
	u16 max_elevation_frame_id;
	enum header_phase phase;
	enum header_type type;
};

union state_machine_player_state_data {
	struct state_machine_player_normal_data normal;
	struct state_machine_player_sliding_tackle_data sliding_tackle;
	struct state_machine_player_fall_data fall;
	struct state_machine_player_header_data header;
	struct state_machine_default_empty_data goalie;
};

define_state_machine(player);
define_state_machine_functions(player);

enum state_machine_goalie_state {
	GS_NONE,
	GS_PROTECT_GOAL,
	GS_DIVE,
	GS_HOLD_BALL,
	GS_JUMP,
	GS_COUNT
};

extern const char *state_machine_goalie_state_string[GS_COUNT];

struct state_machine_goalie_hold_ball_data {
	fg_time_t hold_start_time;
};

struct state_machine_goalie_animation_data {
	f64 animation_start_time;
	f64 animation_first_phase_end_time;
	f64 animation_end_time;
	b32 animation_first_phase_completed;
	b32 animation_second_phase_started;
	b32 animation_completed;
	b32 ball_saved;
	b32 ball_held;
};

union state_machine_goalie_state_data {
	struct state_machine_default_empty_data protect_goal;
	struct state_machine_goalie_animation_data dive;
	struct state_machine_goalie_hold_ball_data hold_ball;
	struct state_machine_goalie_animation_data jump;
};

enum player_collider_type {
	PCT_PLAYER,
	PCT_GOALIE,
	PCT_DIVE,
	PCT_TACKLE_1,
	PCT_TACKLE_2,
	PCT_HEADER,
	PCT_BODY,
	PCT_COUNT
};

define_state_machine(goalie);
define_state_machine_functions(goalie);

#define check_is_player_and_declare_components(entity)                                             \
	struct component_transform *transform = &entity->transform;                                    \
	struct component_physics *physics = &entity->physics;                                          \
	struct component_sprite *sprite = &entity->sprite;                                             \
	struct component_player *player = &entity->player;                                             \
	struct component_collider *collider = &entity->collider;                                       \
	if (entity->header.alive && transform->header.enabled && physics->header.enabled &&            \
		sprite->header.enabled && player->header.enabled && collider->header.enabled)

#define check_is_goalie_and_declare_components(entity)                                             \
	struct component_transform *transform = &entity->transform;                                    \
	struct component_physics *physics = &entity->physics;                                          \
	struct component_sprite *sprite = &entity->sprite;                                             \
	struct component_player *player = &entity->player;                                             \
	struct component_goalie *goalie = &entity->goalie;                                             \
	struct component_collider *collider = &entity->collider;                                       \
	if (entity->header.alive && transform->header.enabled && physics->header.enabled &&            \
		sprite->header.enabled && player->header.enabled && goalie->header.enabled &&              \
		collider->header.enabled)

struct player_ball_reachability_prediction {
	b32 can_reach_it;
	v3 future_pos;
	fg_time_t future_time;
};

enum player_ball_possession {

	PBP_ON_THE_BALL,
	PBP_IN_POSSESSION,
	PBP_OFF_THE_BALL
};

struct collision_information {
	b32 is_colliding;
	v2 direction;
	f32 force;
};

struct nearest_player {
	struct entity *entity;
	f32 distance;
};

struct player_characteristics {
	u8 speed;		 // 1-5, top speed
	u8 acceleration; // 1-5, how fast they reach top speed / change direction
	u8 strength;	 // 1-5, movement force and resistance to drag
	u8 weight;		 // 1-5, mass for collisions (light=1 to heavy=5)
};

struct player_derived_physics {
	f32 movement_force_multiplier;
	f32 max_velocity_multiplier;
	f32 drag_multiplier;
	f32 accel_ramp_up;
	f32 decel_ramp_down;
};

struct player_config {
	v2 position;
	b32 is_goalie;
	enum team_side team_side;
	u8 team_formation_position_idx;
	struct player_characteristics characteristics;
};

struct entity *entity_player_create(struct player_config config, struct game_state *game_state);

void entity_player_physics_apply_force(struct entity *player_entity,
									   Vector2 steering,
									   struct game_state *game_state);

void entity_player_physics_apply_velocity(struct entity *player_entity,
										  struct game_state *game_state);

void entity_player_anim_calculate_running_frame(struct entity *player_entity,
												asset_sprite_frame_id_t *stand_frames,
												asset_animation_id_t *run_animations,
												v2 direction,
												f32 velocity_threshold,
												struct game_state *game_state);

struct collision_information
entity_player_is_colliding_with_ball(struct entity *player_entity,
									 enum player_collider_type collider_type,
									 v3 ball_position,
									 v3 ball_velocity,
									 f32 ball_radius,
									 f32 max_height);

b32 entity_player_goalie_should_dive(struct entity *goalie_entity,
									 struct entity *ball_entity,
									 enum save_direction *dive_direction,
									 enum save_height *dive_height,
									 f32 *dive_speed,
									 f32 *dive_distance,
									 struct game_state *game_state);

void entity_player_gained_possession(struct entity *player_entity, struct game_state *game_state);

enum player_ball_possession entity_player_ball_possession(struct entity *player_entity,
														  struct entity *ball_entity,
														  struct game_state *game_state);

void entity_player_selected_for_pass(struct entity *player_entity, struct game_state *game_state);

struct player_ball_reachability_prediction entity_player_can_reach_future_ball_position(
	struct entity *seeker_entity,
	f32 seeker_max_speed,
	v3 *future_ball_positions,
	b32(predicate)(v3 pos_to_check, struct entity *seeker_entity, struct game_state *game_state),
	struct game_state *game_state);

struct entity *entity_player_find_pass_target(struct entity *player_on_the_ball_entity,
											  f32 detection_angle,
											  f32 max_distance,
											  struct game_state *game_state);

struct nearest_player *find_nearest_player(v2 position,
										   enum team_side team_side,
										   struct list_entity_ids exclusion_list,
										   struct game_state *game_state);

struct nearest_player *entity_player_find_nearest_facing_opponent(struct entity *player_entity,
																  struct game_state *game_state);

struct list_entities entity_player_find_tackled_opponents(struct entity *player_entity,
														  struct game_state *game_state);

b32 entity_player_is_trajectory_clear(v2 passer_position,
									  v2 target_position,
									  enum team_side opposing_team,
									  struct game_state *game_state);

struct player_derived_physics entity_player_derive_physics(const struct player_characteristics *c);

void entity_player_header_hit(struct entity *player_entity, struct game_state *game_state);

#endif // !ENTITY_PLAYER_H
