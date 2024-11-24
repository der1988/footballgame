#ifndef ENTITIES_PLAYER_H
#define ENTITIES_PLAYER_H

#include "components.h"
#include "state_machine.h"

struct entity;

struct player_config {
    v2 position;
    b32 is_goalie;
    i8 team_idx;
    u8 team_formation_position_idx;
};

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

static const char *state_machine_player_state_string[] = {
    "none", "normal", "throw_in", "header", "tackle", "fall", "goalie"};

declare_state_machine_enter_and_exit(player);

struct state_machine_player_normal_data {
    enum direction running_action_direction;
};

struct state_machine_player_tackle_data {
    time_t start_time;
    b32 touched_ball;
    b32 touched_player;
};

struct state_machine_player_fall_data {
    time_t animation_start_time;
    time_t animation_end_time;
    b32 animation_completed;
};

union state_machine_player_state_data {
    struct state_machine_player_normal_data normal;
    struct state_machine_player_tackle_data tackle;
    struct state_machine_player_fall_data fall;
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

static const char *state_machine_goalie_state_string[] = {
    "none", "protect_goal", "dive", "hold_ball"};

declare_state_machine_enter_and_exit(goalie);

struct state_machine_goalie_hold_ball_data {
    time_t hold_start_time;
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

#endif // !ENTITIES_PLAYER_H
