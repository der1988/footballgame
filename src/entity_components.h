#ifndef ENTITY_COMPONENTS_H
#define ENTITY_COMPONENTS_H

#include "action.h"
#include "asset_sprite_frame.h"
#include "collision.h"
#include "football.h"
#include "render.h"
#include "types.h"
#include "entity_player.h"
#include "entity_ball.h"

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

struct component_collider {
	struct component_header header;
	struct collider *colliders;
	u16 num_colliders;
};

enum camera_type {
	CAM_MAIN,
	CAM_COUNT
};

extern const char *camera_type_to_string[CAM_COUNT];

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
	// When a tackle last played the ball off him
	fg_time_low_prec_t last_tackle_dispossession_time;
	fg_time_low_prec_t last_header_time;
	fg_time_low_prec_t last_frame_update_time;
	fg_time_low_prec_t last_possession_time;
	u16 last_running_frame_idx;
	u8 team_formation_position_idx;
	enum team_side team_side;
	struct player_characteristics characteristics;
	struct player_constants constants; // characteristics resolved against base tuning; cached
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
	// Steering was set by an assist rather than the stick
	b32 steering_assisted;
	f32 shot;
	f32 chip_shot;
	f32 pass;
	// The team mate a pass, throw or header goes to. EID_NOT_VALID sends it along the steering.
	entity_id_t pass_target_entity_id;
	f32 long_pass;
	f32 long_driven_pass;
	b32 header_shot;
	b32 header_pass;
	b32 tackle;
	// The player in possession the tackle goes in on
	entity_id_t tackle_victim_entity_id;
	b32 sliding_tackle;
	b32 stop;
	// The keeper sets the ball down to carry on with his feet
	b32 put_down;
	// The keeper is winding up a throw, so he holds the throwing pose
	b32 charging_throw;
	// The keeper goes up for a ball he can reach where he stands
	b32 jump;
	// The keeper dives, the way dive_data describes
	b32 dive;
	struct goalie_dive dive_data;
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
	struct future_position future_positions[FUTURE_POSITION_COUNT];
	fg_time_t last_bounce_time;
	b32 is_on_the_ground;
	// Where the ball was before the last physics step: x, height, y
	v3 previous_position;

	// Touch
	fg_time_t last_touch_time;
	entity_id_t last_touch_entity_id;
	fg_time_t first_player_possession_time;

	// Plays: the ball played on purpose (shots, passes, headers, slides)
	fg_time_t last_play_time;
	entity_id_t last_play_entity_id;
	enum ball_play_type last_play_type;
	// When the ball last got away from whoever had it: every kick, plus standing tackles and
	// deflections. Nobody is in possession of it until it is touched again.
	fg_time_t last_loose_time;
	// Set when a player plays the ball with his feet or his goalie releases it, for that team's
	// goalie; cleared by any touch of the other team and by a header from a team mate.
	enum ball_not_pickable_by not_pickable_by;

	// Passes
	entity_id_t pass_target_entity_id;

	// Tackles
	fg_time_t last_tackle_time;
	entity_id_t last_tackle_entity_id;

	// After touch
	struct {
		b32 active;
		fg_time_t activated_at;
		v2 activation_direction;
		v2 selected_side;
	} after_touch;

	// Animation
	u16 last_running_frame_idx;
	fg_time_t last_frame_update_time;
};

struct component_goal {
	struct component_header header;
	enum pitch_side side;
	b32 scored;
};

struct component_goalie {
	struct component_header header;
	struct state_machine_goalie state_machine;
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

#endif // !ENTITY_COMPONENTS_H
