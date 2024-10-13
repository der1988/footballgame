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
    running_action_idx_t running_action_idx;
};

struct Entity;

// START - Common

AssetsAnimationId entities_common_animation_id_from_name(char *entity_name, char *animation_name,
							 struct GameState *game_state);
enum Direction entities_common_calculate_direction(Vector2 direction);

bool entities_common_can_reach_position_in_time(v2 current_position, v2 position_to_reach,
						time_t time, f32 max_speed);

bool entities_common_can_reach_future_ball_position(
    struct Entity *seeker_entity, f32 seeker_max_speed, v3 *future_ball_positions,
    bool(predicate)(v3 pos_to_check, struct Entity *seeker_entity, struct GameState *game_state),
    v3 *future_pos, time_t *future_time, struct GameState *game_state);

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
void entities_ball_set_position_and_velocity(entity_id_t ball_entity_id, v3 pos, v3 velocity,
					     struct GameState *game_state);
void entities_ball_control_precision(entity_id_t ball_entity_id,
				     entity_id_t player_on_the_ball_entity_id, v2 player_velocity,
				     v2 position_to_seek, f32 control_quality,
				     struct GameState *game_state);
void entities_ball_control_running(entity_id_t ball_entity_id,
				   entity_id_t player_on_the_ball_entity_id, v2 control_direction,
				   v2 player_velocity, f32 control_quality,
				   struct GameState *game_state);
void entities_ball_shoot(entity_id_t ball_entity_id, entity_id_t kicker_entity_id, v2 direction,
			 f32 power, struct GameState *game_state);
void entities_ball_short_pass(entity_id_t ball_entity_id, entity_id_t kicker_entity_id,
			      v3 direction, f32 power, b32 auto_pass, struct GameState *game_state);
void entities_ball_long_pass(entity_id_t ball_entity_id, entity_id_t kicker_entity_id, v2 direction,
			     f32 power, struct GameState *game_state);
void entities_ball_held_by_goalie(entity_id_t ball_entity_id, entity_id_t goalie_entity_id,
				  struct GameState *game_state);
void entities_ball_saved_by_goalie(entity_id_t ball_entity_id, entity_id_t goalie_entity_id,
				   struct GameState *game_state);
void entities_ball_back_in_play(entity_id_t ball_entity_id, struct GameState *game_state);
void entities_ball_in_goal(entity_id_t ball_entity_id, struct GameState *game_state);

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
    time_t hold_start_time;
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
void entities_player_apply_force(entity_id_t player_entity_id, Vector2 steering,
				 struct GameState *game_state);
void entities_player_apply_velocity(entity_id_t player_entity_id, struct GameState *game_state);
void entities_player_calculate_running_frame(entity_id_t player_entity_id, v2 direction,
					     f32 velocity_threshold, struct GameState *game_state);
Rectangle entities_player_calculate_dive_collision_rectangle(v2 pos,
							     struct Collider *dive_collider);
bool entities_player_is_diving_goalie_colliding_with_ball(struct Entity *goalie_entity,
							  v3 ball_position, f32 ball_radius,
							  f32 max_height);
bool entities_player_is_standing_goalie_colliding_with_ball(struct Entity *goalie_entity,
							    v3 ball_position, f32 ball_radius,
							    f32 max_height);
bool entities_player_should_dive(struct Entity *goalie_entity, struct Entity *ball_entity,
				 enum SaveDirection *dive_direction, enum SaveHeight *dive_height,
				 f32 *dive_speed, f32 *dive_distance, struct GameState *game_state);

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
    entity_id_t id;
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
    memory_arena_handle_t *arena_handle;
    struct Entity *entities;
    u32 available_slots;
    u32 max_slots;
    u32 last_used_slot;
} EntityRepository;

EntityRepository *entities_repository_init(memory_arena_handle_t *memory_arena_handle);
void entities_repository_destroy(EntityRepository *handle);

struct Entity *entities_entity_init(EntityRepository *handle, EntityType type, char *name_format);
void entities_entity_destroy(EntityRepository *handle, entity_id_t id);
struct Entity *entities_entity_get(EntityRepository *handle, entity_id_t id);
bool entities_is_entity_alive_and_has_components(struct Entity *entity, u32 count, ...);

#endif
