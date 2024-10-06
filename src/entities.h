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
#define C_TRANSFORM		      offsetof(struct Entity, transform)
#define C_PHYSICS		      offsetof(struct Entity, physics)
#define C_SPRITE		      offsetof(struct Entity, sprite)
#define C_PLAYER		      offsetof(struct Entity, player)
#define C_GOALIE		      offsetof(struct Entity, goalie)
#define C_COLLIDER		      offsetof(struct Entity, collider)
#define C_CAMERA		      offsetof(struct Entity, camera)
#define C_PHYSICS_3D		      offsetof(struct Entity, physics_3d)
#define C_ACTION		      offsetof(struct Entity, action)
#define C_TARGET		      offsetof(struct Entity, target)
#define C_BALL			      offsetof(struct Entity, ball)
#define C_GOAL			      offsetof(struct Entity, goal)

#define for_each_entity(item, entity_repo)                                                         \
    struct Entity *item = entity_repo->entities;                                                   \
    for (usize item##_id = 0; entity_id < entity_repo->max_slots; ++item##_id, item++)

#define declare_entity_component_ext(entity, type, member, name)                                   \
    struct Component##type *name = &entity->member

#define declare_entity_component(entity, type, member)                                             \
    declare_entity_component_ext(entity, type, member, member)

#define declare_ball_entity_components(entity)                                                     \
    declare_entity_component_ext(entity, Transform, transform, ball_transform);                    \
    declare_entity_component_ext(entity, Physics3d, physics_3d, ball_physics_3d);                  \
    declare_entity_component_ext(entity, Collider, collider, ball_collider);                       \
    declare_entity_component_ext(entity, Ball, ball, ball)

struct StateMachineRunningActionState
{
    RunningActionIdx running_action_idx;
};

struct Entity;

// START - Common

AssetsAnimationId entities_common_animation_id_from_name(char *entity_name, char *animation_name,
							 struct GameState *game_state);
enum Direction entities_common_calculate_direction(Vector2 direction);

// END

// START - Camera

struct Entity *entities_camera_create(struct GameState *game_state);

// END - Camera

// START - Ball

struct BallConfig
{
    v2 position;
};

enum StateMachineBallState
{
    BS_NONE,
    BS_IN_PLAY,
    BS_OUT_OF_PLAY,
    BS_GK_HANDS,
    BS_COUNT
};

static char const *StateMachineBallStateString[] = {"none", "in_play", "out_of_play", "gk_hands",
						    "count"};

declare_state_machine_enter_and_exit(ball);

union StateMachineBallStateData
{
    struct StateMachineDefaultEmptyData in_play;
    struct StateMachineDefaultEmptyData out_of_play;
    struct StateMachineDefaultEmptyData gk_hands;
};

define_state_machine(Ball);
define_state_machine_functions(Ball, ball);

struct Entity *entities_ball_create(struct BallConfig config, struct GameState *game_state);
void entities_ball_set_position_and_velocity(EntityId ball_entity_id, v3 pos, v3 velocity,
					     struct GameState *game_state);
void entities_ball_control_precision(EntityId ball_entity_id, EntityId player_on_the_ball_entity_id,
				     v2 player_velocity, v2 position_to_seek, f32 control_quality,
				     struct GameState *game_state);
void entities_ball_control_running(EntityId ball_entity_id, EntityId player_on_the_ball_entity_id,
				   v2 control_direction, v2 player_velocity, f32 control_quality,
				   struct GameState *game_state);
void entities_ball_shoot(EntityId ball_entity_id, EntityId kicker_entity_id, v2 direction,
			 f32 power, struct GameState *game_state);
void entities_ball_short_pass(EntityId ball_entity_id, EntityId kicker_entity_id, v3 direction,
			      f32 power, b32 auto_pass, struct GameState *game_state);
void entities_ball_long_pass(EntityId ball_entity_id, EntityId kicker_entity_id, v2 direction,
			     f32 power, struct GameState *game_state);
void entities_ball_held_by_goalie(EntityId ball_entity_id, EntityId goalie_entity_id,
				  struct GameState *game_state);
void entities_ball_saved_by_goalie(EntityId ball_entity_id, EntityId goalie_entity_id,
				   struct GameState *game_state);
void entities_ball_back_in_play(EntityId ball_entity_id, struct GameState *game_state);
void entities_ball_in_goal(EntityId ball_entity_id, struct GameState *game_state);

// END - Ball

// START - Player

typedef struct PlayerConfig
{
    v2 position;
    b32 is_goalie;
    enum PitchSide side;
    u8 team_formation_position_idx;
} PlayerConfig;

typedef enum StateMachinePlayerState
{
    PS_NONE,
    PS_NORMAL,
    PS_GOALIE,
    PS_COUNT
} StateMachinePlayerState;

static char const *StateMachinePlayerStateString[] = {"none", "normal", "goalie"};

declare_state_machine_enter_and_exit(player);

struct StateMachinePlayerNormalData
{
    enum Direction running_action_direction;
};

union StateMachinePlayerStateData
{
    struct StateMachinePlayerNormalData normal;
    struct StateMachineDefaultEmptyData goalie;
};

define_state_machine(Player);
define_state_machine_functions(Player, player);

typedef enum StateMachineGoalieState
{
    GS_NONE,
    GS_PROTECT_GOAL,
    GS_DIVE,
    GS_HOLD_BALL,
    GS_COUNT
} StateMachineGoalieState;

static char const *StateMachineGoalieStateString[] = {"none", "protect_goal", "dive", "hold_ball"};

declare_state_machine_enter_and_exit(goalie);

typedef struct StateMachineGoalieHoldBallData
{
    Time hold_start_time;
} StateMachineGoalieHoldBallData;

typedef struct StateMachineGoalieDiveData
{
    f64 animation_start_time;
    f64 animation_first_phase_end_time;
    f64 animation_end_time;
    b32 animation_first_phase_completed;
    b32 animation_second_phase_started;
    b32 animation_completed;
    b32 ball_saved;
    b32 ball_held;
} StateMachineGoalieDiveData;

typedef union StateMachineGoalieStateData
{
    struct StateMachineDefaultEmptyData protect_goal;
    struct StateMachineGoalieDiveData dive;
    struct StateMachineGoalieHoldBallData hold_ball;
} StateMachineGoalieStateData;

typedef enum PlayerColliderType
{
    PCT_PLAYER,
    PCT_GOALIE,
    PCT_DIVE,
    PCT_COUNT
} PlayerColliderType;

define_state_machine(Goalie);
define_state_machine_functions(Goalie, goalie);

#define check_is_player_and_declare_components(entity)                                             \
    struct ComponentTransform *transform = &entity->transform;                                     \
    struct ComponentPhysics *physics = &entity->physics;                                           \
    struct ComponentSprite *sprite = &entity->sprite;                                              \
    struct ComponentPlayer *player = &entity->player;                                              \
    struct ComponentCollider *collider = &entity->collider;                                        \
    if (entity->header.alive && transform->header.enabled && physics->header.enabled &&            \
	sprite->header.enabled && player->header.enabled && collider->header.enabled)

#define check_is_goalie_and_declare_components(entity)                                             \
    struct ComponentTransform *transform = &entity->transform;                                     \
    struct ComponentPhysics *physics = &entity->physics;                                           \
    struct ComponentSprite *sprite = &entity->sprite;                                              \
    struct ComponentPlayer *player = &entity->player;                                              \
    struct ComponentGoalie *goalie = &entity->goalie;                                              \
    struct ComponentCollider *collider = &entity->collider;                                        \
    if (entity->header.alive && transform->header.enabled && physics->header.enabled &&            \
	sprite->header.enabled && player->header.enabled && goalie->header.enabled &&              \
	collider->header.enabled)

struct Entity *entities_player_create(PlayerConfig config, struct GameState *game_state);
void entities_player_apply_force(EntityId player_entity_id, Vector2 steering,
				 struct GameState *game_state);
void entities_player_apply_velocity(EntityId player_entity_id, struct GameState *game_state);
void entities_player_calculate_running_frame(EntityId player_entity_id, v2 direction,
					     f32 velocity_threshold, struct GameState *game_state);

// END - Player

// START - Goal

typedef struct GoalConfig
{
    enum PitchSide side;
} GoalConfig;

struct Entity *entities_goal_create(GoalConfig config, struct GameState *game_state);

// END - Goal

// START - Goal net

struct Entity *entities_goal_net_create(struct GameState *game_state);

// END - Goal net

typedef enum EntityType
{
    ET_CAMERA,
    ET_BALL,
    ET_PLAYER,
    ET_GOAL,
    ET_GOAL_NET,
    ET_COUNT
} EntityType;
static const char *EntityTypeToString[ET_COUNT] = {"camera", "ball", "player", "goal", "goal_net"};

typedef struct EntityHeader
{
    b32 alive;
    EntityId id;
    char name[MaxStringSize];
    EntityType type;
} EntityHeader;

struct Entity
{
    EntityHeader header;
    struct ComponentCamera camera;
    struct ComponentTransform transform;
    struct ComponentPhysics physics;
    struct ComponentPhysics3d physics_3d;
    struct ComponentSprite sprite;
    struct ComponentCollider collider;
    struct ComponentAction action;
    struct ComponentTarget target;
    struct ComponentPlayer player;
    struct ComponentBall ball;
    struct ComponentGoal goal;
    struct ComponentGoalie goalie;
};

typedef struct EntityRepository
{
    b32 initialised;
    MemoryArenaHandle *arena_handle;
    struct Entity *entities;
    u32 available_slots;
    u32 max_slots;
    u32 last_used_slot;
} EntityRepository;

EntityRepository *entities_repository_init(MemoryArenaHandle *memory_arena_handle);
void entities_repository_destroy(EntityRepository *handle);

struct Entity *entities_entity_init(EntityRepository *handle, EntityType type, char *name_format);
void entities_entity_destroy(EntityRepository *handle, EntityId id);
struct Entity *entities_entity_get(EntityRepository *handle, EntityId id);
bool entities_is_entity_alive_and_has_components(struct Entity *entity, u32 count, ...);

#endif
