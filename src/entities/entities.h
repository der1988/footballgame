#ifndef ENTITIES_H
#define ENTITIES_H

#include "components.h"
#include "memory.h"
#include "types.h"

#define MAX_ENTITIES		 100
#define ENTITY_NAME_MAX_SIZE 40

#define ENTITY_TYPE_TO_BIT_MASK(TYPE) (1 << TYPE)
#define ENTITY_TYPE_ALL_BIT_MASK	  0xFFFFFFFFFFFFFFFF
#define C_TRANSFORM					  offsetof(struct entity, transform)
#define C_PHYSICS					  offsetof(struct entity, physics)
#define C_SPRITE					  offsetof(struct entity, sprite)
#define C_PLAYER					  offsetof(struct entity, player)
#define C_GOALIE					  offsetof(struct entity, goalie)
#define C_COLLIDER					  offsetof(struct entity, collider)
#define C_CAMERA					  offsetof(struct entity, camera)
#define C_PHYSICS_3D				  offsetof(struct entity, physics3d)
#define C_ACTION					  offsetof(struct entity, action)
#define C_TARGET					  offsetof(struct entity, target)
#define C_BALL						  offsetof(struct entity, ball)
#define C_GOAL						  offsetof(struct entity, goal)

#define FOR_EACH_ENTITY(item, entity_repo)                                                         \
	struct entity *item = entity_repo->entities;                                                   \
	for (usize item##_id = 0; item##_id < entity_repo->max_slots; ++item##_id, item++)

#define LIST_ENTITIES_EMPTY	  (struct list_entities){.array = NULL, .count = 0, .size = 0}
#define LIST_ENTITY_IDS_EMPTY (struct list_entity_ids){.array = NULL, .count = 0, .size = 0}

#define DECLARE_ENTITY_COMPONENT_EXT(entity, type, name)                                           \
	struct component_##type *name = &entity->type

#define DECLARE_ENTITY_COMPONENT(entity, type) DECLARE_ENTITY_COMPONENT_EXT(entity, type, type)

// Forward declaration
struct entity;

struct list_entities {
	struct entity **array;
	usize count;
	usize size;
};

struct list_entity_ids {
	entity_id_t *array;
	usize count;
	usize size;
};

enum entity_type {
	ET_CAMERA,
	ET_BALL,
	ET_PLAYER,
	ET_GOAL,
	ET_GOAL_NET,
	ET_COUNT
};

extern const char *entity_type_to_string[ET_COUNT];

struct entity_header {
	b32 alive;
	entity_id_t id;
	char name[ENTITY_NAME_MAX_SIZE];
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

struct entity_repository *entity_repository_init(memory_arena_handle_t *memory_arena_handle);
void entity_repository_destroy(struct entity_repository *handle);

struct entity *entity_init(struct entity_repository *handle,
						   enum entity_type type,
						   char *name_format);
void entity_destroy(struct entity_repository *handle, entity_id_t id);
struct entity *entity_get(struct entity_repository *handle, entity_id_t id);
b32 entity_is_alive_and_has_components(struct entity *entity, u32 count, ...);

#endif // !ENTITIES_H
