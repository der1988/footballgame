#ifndef ENTITIES_H
#define ENTITIES_H

#include "components.h"
#include "entities/state_machine.h"
#include "football.h"
#include "game.h"
#include "input.h"
#include "memory.h"
#include "state_machine.h"
#include "types.h"

#define MAX_ENTITIES 100

#define ENTITY_TYPE_TO_BIT_MASK(TYPE) (1 << TYPE)
#define ENTITY_TYPE_ALL_BIT_MASK      0xFFFFFFFFFFFFFFFF
#define C_TRANSFORM		      offsetof(struct entity, transform)
#define C_PHYSICS		      offsetof(struct entity, physics)
#define C_SPRITE		      offsetof(struct entity, sprite)
#define C_PLAYER		      offsetof(struct entity, player)
#define C_GOALIE		      offsetof(struct entity, goalie)
#define C_COLLIDER		      offsetof(struct entity, collider)
#define C_CAMERA		      offsetof(struct entity, camera)
#define C_PHYSICS_3D		      offsetof(struct entity, physics3d)
#define C_ACTION		      offsetof(struct entity, action)
#define C_TARGET		      offsetof(struct entity, target)
#define C_BALL			      offsetof(struct entity, ball)
#define C_GOAL			      offsetof(struct entity, goal)

#define for_each_entity(item, entity_repo)                                                         \
    struct entity *item = entity_repo->entities;                                                   \
    for (usize item##_id = 0; entity_id < entity_repo->max_slots; ++item##_id, item++)

#define declare_entity_component_ext(entity, type, name)                                           \
    struct component_##type *name = &entity->type

#define declare_entity_component(entity, type) declare_entity_component_ext(entity, type, type)

#define declare_ball_entity_components(entity)                                                     \
    declare_entity_component_ext(entity, transform, ball_transform);                               \
    declare_entity_component_ext(entity, physics3d, ball_physics_3d);                              \
    declare_entity_component_ext(entity, collider, ball_collider);                                 \
    declare_entity_component_ext(entity, ball, ball)

// Forward declaration
struct entity;

// START - Common

struct state_machine_running_action_state {
    running_action_idx_t running_action_idx;
};

assets_animation_id_t entities_common_animation_id_from_name(const char *entity_name,
							     const char *animation_name,
							     struct game_state *game_state);
enum direction entities_common_calculate_direction(Vector2 direction);

b32 entities_common_can_reach_position_in_time(v2 current_position,
					       v2 position_to_reach,
					       time_t time,
					       f32 max_speed);

b32 entities_common_can_reach_future_ball_position(struct entity *seeker_entity,
						   f32 seeker_max_speed,
						   v3 *future_ball_positions,
						   b32(predicate)(v3 pos_to_check,
								  struct entity *seeker_entity,
								  struct game_state *game_state),
						   v3 *future_pos,
						   time_t *future_time,
						   struct game_state *game_state);

// END

// START - Camera

struct entity *entities_camera_create(struct game_state *game_state);

// END - Camera

// START - Ball

struct ball_config {
    v2 position;
};

enum state_machine_ball_state {
    BS_NONE,
    BS_IN_PLAY,
    BS_OUT_OF_PLAY,
    BS_GK_HANDS,
    BS_COUNT
};

static const char *state_machine_ball_state_string[] = {
    "none", "in_play", "out_of_play", "gk_hands", "count"};

declare_state_machine_enter_and_exit(ball);

union state_machine_ball_state_data {
    struct state_machine_default_empty_data in_play;
    struct state_machine_default_empty_data out_of_play;
    struct state_machine_default_empty_data gk_hands;
};

define_state_machine(ball);
define_state_machine_functions(ball);

struct entity *entities_ball_create(struct ball_config config, struct game_state *game_state);
void entities_ball_set_position_and_velocity(entity_id_t ball_entity_id,
					     v3 pos,
					     v3 velocity,
					     struct game_state *game_state);
void entities_ball_control_precision(entity_id_t ball_entity_id,
				     entity_id_t player_on_the_ball_entity_id,
				     v2 player_velocity,
				     v2 position_to_seek,
				     f32 difficulty,
				     struct game_state *game_state);
void entities_ball_control_running(entity_id_t ball_entity_id,
				   entity_id_t player_on_the_ball_entity_id,
				   v2 control_direction,
				   v2 player_velocity,
				   f32 difficulty,
				   struct game_state *game_state);
void entities_ball_shoot(entity_id_t ball_entity_id,
			 entity_id_t kicker_entity_id,
			 v2 direction,
			 f32 height,
			 f32 power,
			 struct game_state *game_state);
void entities_ball_short_pass(entity_id_t ball_entity_id,
			      entity_id_t kicker_entity_id,
			      v3 direction,
			      f32 power,
			      b32 auto_pass,
			      struct game_state *game_state);
void entities_ball_long_pass(entity_id_t ball_entity_id,
			     entity_id_t kicker_entity_id,
			     v2 direction,
			     f32 power,
			     struct game_state *game_state);
void entities_ball_held_by_goalie(entity_id_t ball_entity_id,
				  entity_id_t goalie_entity_id,
				  struct game_state *game_state);
void entities_ball_saved_by_goalie(entity_id_t ball_entity_id,
				   entity_id_t goalie_entity_id,
				   struct game_state *game_state);
void entities_ball_back_in_play(entity_id_t ball_entity_id, struct game_state *game_state);
void entities_ball_in_goal(entity_id_t ball_entity_id, struct game_state *game_state);
void entities_ball_set_player_on_the_ball(struct component_ball *ball,
					  entity_id_t player_on_the_ball_entity_id,
					  struct game_state *game_state);

// END - Ball

// START - Player

struct player_config {
    v2 position;
    b32 is_goalie;
    enum pitch_side side;
    u8 team_formation_position_idx;
};

enum state_machine_player_state {
    PS_NONE,
    PS_NORMAL,
    PS_THROW_IN,
    PS_HEADER,
    PS_SLIDING_TACKLE,
    PS_GOALIE,
    PS_COUNT
};

static const char *state_machine_player_state_string[] = {"none", "normal", "goalie"};

declare_state_machine_enter_and_exit(player);

struct state_machine_player_normal_data {
    enum direction running_action_direction;
};

union state_machine_player_state_data {
    struct state_machine_player_normal_data normal;
    struct state_machine_default_empty_data goalie;
};

define_state_machine(player);
define_state_machine_functions(player);

enum state_machine_goalie_state {
    GS_NONE,
    GS_PROTECT_GOAL,
    GS_DIVE,
    GS_HOLD_BALL,
    GS_COUNT
};

static const char *state_machine_goalie_state_string[] = {
    "none", "protect_goal", "dive", "hold_ball"};

declare_state_machine_enter_and_exit(goalie);

struct state_machine_goalie_hold_ball_data {
    time_t hold_start_time;
};

struct state_machine_goalie_dive_data {
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
    struct state_machine_goalie_dive_data dive;
    struct state_machine_goalie_hold_ball_data hold_ball;
};

enum player_collider_type {
    PCT_PLAYER,
    PCT_GOALIE,
    PCT_DIVE,
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

struct entity *entities_player_create(struct player_config config, struct game_state *game_state);
void entities_player_apply_force(entity_id_t player_entity_id,
				 Vector2 steering,
				 struct game_state *game_state);
void entities_player_apply_velocity(entity_id_t player_entity_id, struct game_state *game_state);
void entities_player_calculate_running_frame(entity_id_t player_entity_id,
					     v2 direction,
					     f32 velocity_threshold,
					     struct game_state *game_state);
b32 entities_player_is_colliding_with_ball(struct entity *entity,
					   enum player_collider_type collider_type,
					   v3 ball_position,
					   f32 ball_radius,
					   f32 max_height);
b32 entities_player_should_dive(struct entity *goalie_entity,
				struct entity *ball_entity,
				enum save_direction *dive_direction,
				enum save_height *dive_height,
				f32 *dive_speed,
				f32 *dive_distance,
				struct game_state *game_state);

// END - Player

// START - Goal

struct goal_config {
    enum pitch_side side;
    v2 center;
};

struct entity *entities_goal_create(struct goal_config config, struct game_state *game_state);

// END - Goal

// START - Goal net

struct entity *entities_goal_net_create(struct game_state *game_state);

// END - Goal net

enum entity_type {
    ET_CAMERA,
    ET_BALL,
    ET_PLAYER,
    ET_GOAL,
    ET_GOAL_NET,
    ET_COUNT
};
static const char *entity_type_to_string[ET_COUNT] = {
    "camera", "ball", "player", "goal", "goal_net"};

struct entity_header {
    b32 alive;
    entity_id_t id;
    char name[MaxStringSize];
    enum entity_type type;
};

struct entity {
    struct entity_header header;
    struct component_camera camera;
    struct component_transform transform;
    struct component_physics physics;
    struct component_physics3d physics3d;
    struct component_sprite sprite;
    struct component_collider collider;
    struct component_action action;
    struct component_target target;
    struct component_player player;
    struct component_ball ball;
    struct component_goal goal;
    struct component_goalie goalie;
};

struct entity_repository {
    b32 initialised;
    memory_arena_handle_t *arena_handle;
    struct entity *entities;
    u32 available_slots;
    u32 max_slots;
    u32 last_used_slot;
};

struct entity_repository *entities_repository_init(memory_arena_handle_t *memory_arena_handle);
void entities_repository_destroy(struct entity_repository *handle);

struct entity *entities_entity_init(struct entity_repository *handle,
				    enum entity_type type,
				    char *name_format);
void entities_entity_destroy(struct entity_repository *handle, entity_id_t id);
struct entity *entities_entity_get(struct entity_repository *handle, entity_id_t id);
b32 entities_is_entity_alive_and_has_components(struct entity *entity, u32 count, ...);

#endif
