#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "action.h"
#include "as_sprite_frame.h"
#include "football.h"
#include "render.h"
#include "constants.h"
#include "types.h"
#include "e_player.h"
#include "e_ball.h"

#define MAX_CONCURRENT_ACTIONS 5

// Forward declarations
struct entity;
struct state_machine_ball;
struct state_machine_player;

struct component_header {
	b32 enabled;
};

struct component_transform {
	struct component_header header;
	struct component_transform *parent;
	v2 position;
	f32 rotation;
	v2 scale;
	f32 virtual_height;
};

struct component_physics {
	struct component_header header;
	f32 mass;
	v2 accel;
	v2 velocity;
	f32 max_velocity;
	f32 max_force;
};

struct component_physics3d {
	struct component_header header;
	f32 mass;
	v3 accel;
	v3 velocity;
	f32 max_velocity;
	f32 max_force;
};

struct component_sprite {
	struct component_header header;
	color_t color;
	b32 flipX;
	b32 flipY;
	enum sprite_orientation orientation;
	enum sprite_layer layer;
	i16 order_in_layer;
	v2 pivot_override;
	asset_sprite_frame_id_t frame_id;
	b32 draw_shadow;
	color_t shadow_color;
	v2 shadow_offset;
	v2 shadow_scale;
	void (*before_draw)(struct entity *, struct game_state *);
	void (*after_draw)(struct entity *, struct game_state *);
};

struct component_action {
	struct component_header header;
	struct action_runner running_actions[MAX_CONCURRENT_ACTIONS];
};

struct component_target {
	struct component_header header;
	entity_id_t entity_id;
};

struct collider {
	enum {
		CS_CIRCLE,
		CS_RECTANGLE
	} shape;
	union {
		struct {
			f32 radius;
			v2 offset;
		} circle;
		struct {
			v2 size;
			v2 offset;
		} rectangle;
	} props;
};

struct component_collider {
	struct component_header header;
	struct collider *colliders;
	u16 num_colliders;
};

enum camera_type {
	CAM_MAIN,
	CAM_COUNT
};

struct component_camera {
	struct component_header header;
	enum camera_type camera;
	f32 slowing_distance;
	v2 offset;
};

struct component_player {
	struct component_header header;
	struct state_machine_player state_machine;
	b32 control_ball;
	fg_time_t time_spent_in_precision_control;
	fg_time_t time_spent_in_run_control;
	f32 current_max_velocity;
	fg_time_low_prec_t last_kick_time;
	fg_time_low_prec_t last_frame_update_time;
	fg_time_low_prec_t last_possession_time;
	fg_time_low_prec_t last_selected_for_pass_time;
	u16 last_running_frame_idx;
	u8 team_formation_position_idx;
	enum team_side team_side;
	struct player_characteristics characteristics;
	fg_time_low_prec_t last_body_collision_time;
	f32 last_body_collision_impulse;
	b32 is_colliding_with_ball;
	v2 collision_direction;
	f32 collision_force;
	b32 kicked_ball;
};

struct component_player_controls {
	struct component_header header;
	v2 steering;
	f32 shot;
	f32 pass;
	b32 automatic_pass;
	f32 long_pass;
	b32 header_shot;
	b32 header_pass;
	b32 tackle;
	b32 sliding_tackle;
	b32 stop;
};

enum future_position_type {
	FUTURE_POSITION_BOUNCE,
	FUTURE_POSITION_REACHABLE_TO_PLAYER,
	FUTURE_POSITION_REACHABLE_TO_GOALIE,
	FUTURE_POSITION_COUNT,
};

struct future_position {
	b32 available;
	v3 position;
	u32 dts;
};

struct component_ball {
	struct component_header header;
	struct state_machine_ball state_machine;

	// Lookahead and bounces
	v3 lookahead_positions[BALL_MAX_LOOKAHEADS];
	struct future_position future_positions[FUTURE_POSITION_COUNT];
	fg_time_t last_bounce_time;
	b32 is_on_the_ground;

	// Touch
	fg_time_t last_touch_time;
	entity_id_t last_touch_entity_id;
	fg_time_t first_player_possession_time;

	// Kicks
	fg_time_t last_kick_time;
	entity_id_t last_kicker_entity_id;

	// Passes
	entity_id_t pass_target_entity_id;

	// Tackles
	fg_time_t last_tackle_time;
	entity_id_t last_tackle_entity_id;

	// Animation
	u16 last_running_frame_idx;
	fg_time_t last_frame_update_time;
};

struct component_goal {
	struct component_header header;
	enum pitch_side side;
	fg_time_t hit_post_time;
	b32 scored;
};

struct component_goalie {
	struct component_header header;
	struct state_machine_goalie state_machine;
	enum save_direction dive_direction;
	enum save_height dive_height;
	f32 dive_distance;
	f32 dive_speed;
	fg_time_t last_frame_update_time;
	fg_time_t last_save_time;
};

running_action_idx_t component_action_run(entity_id_t id,
										  struct action *action,
										  struct game_state *game_state);
void component_action_stop(entity_id_t id,
						   running_action_idx_t running_action_idx,
						   struct game_state *game_state);

void component_transform_local_to_world(struct component_transform *transform,
										v2 *position,
										f32 *rotation,
										v2 *scale);

#endif // !COMPONENTS
