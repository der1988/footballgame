#ifndef ENTITIES_H
#define ENTITIES_H

#include "components.h"
#include "types.h"

#include "entities/e_ball.h"
#include "entities/e_player.h"

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

#define FOR_EACH_ENTITY(item, entity_repo)                                                         \
    struct entity *item = entity_repo->entities;                                                   \
    for (usize item##_id = 0; entity_id < entity_repo->max_slots; ++item##_id, item++)

#define DECLARE_ENTITY_COMPONENT_EXT(entity, type, name)                                           \
    struct component_##type *name = &entity->type

#define DECLARE_ENTITY_COMPONENT(entity, type) DECLARE_ENTITY_COMPONENT_EXT(entity, type, type)

#define DECLARE_BALL_ENTITY_COMPONENTS(entity)                                                     \
    DECLARE_ENTITY_COMPONENT_EXT(entity, transform, ball_transform);                               \
    DECLARE_ENTITY_COMPONENT_EXT(entity, physics3d, ball_physics_3d);                              \
    DECLARE_ENTITY_COMPONENT_EXT(entity, collider, ball_collider);                                 \
    DECLARE_ENTITY_COMPONENT_EXT(entity, ball, ball)

// Forward declaration
struct entity;

// START - Common

struct state_machine_running_action_state {
    running_action_idx_t running_action_idx;
};

enum sprite_orientation entities_common_calculate_orientation(v2 direction);

b32 entities_common_can_reach_position_in_time(v2 current_position,
					       v2 position_to_reach,
					       time_t time,
					       f32 max_speed);

// END - Common

// START - Camera

struct entity *entities_camera_create(struct game_state *game_state);

// END - Camera

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
    struct component_player_controls player_controls;
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
