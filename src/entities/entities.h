#ifndef ENTITIES_H
#define ENTITIES_H

#include "../components/components.h"
#include "../entities/state_machine.h"
#include "../input/input.h"
#include "../memory/memory.h"
#include "config.h"
#include "entities/state_machine.h"
#include "game.h"
#include "types.h"

#define MAX_ENTITIES 100

#define EntityTypeToBitMask(TYPE) (1 << TYPE)
#define EntityTypeAllBitMask 0xFFFFFFFFFFFFFFFF
#define EntityTypeCheckMask(type, mask) (EntityTypeToBitMask(type) & mask)

#define EntityForEach(entity_repo, item, entity_id)                                                \
    Entity *item = entity_repo->entities;                                                          \
    for (usize entity_id = 0; entity_id < entity_repo->max_slots; ++entity_id, item++)

#define DeclareStateMachineEnterAndExit(ENTITY_NAME)                                               \
    void state_machine_##ENTITY_NAME##_on_enter(struct Entity *ENTITY_NAME,                        \
                                                GameState *game_state);                            \
    void state_machine_##ENTITY_NAME##_on_exit(struct Entity *ENTITY_NAME, GameState *game_state);

// Shared State Data
typedef struct {
    RunningActionIdx running_action_idx;
} StateMachineRunningActionState;

struct Entity;

// START - Common

AssetsAnimationId entities_common_animation_id_from_name(char *entity_name, char *animation_name,
                                                         GameState *game_state);
Direction entities_common_calculate_direction(Vector2 direction);

// END

// START - Camera

struct Entity *entities_camera_create(GameState *game_state);

// END - Camera

// START - Ball

typedef enum { BS_NONE, BS_IN_PLAY, BS_OUT_OF_PLAY, BS_GK_HANDS, BS_COUNT } StateMachineBallState;

static char const *StateMachineBallStateString[] = {"none", "in_play", "out_of_play", "gk_hands",
                                                    "count"};

DeclareStateMachineEnterAndExit(ball);

typedef union {
    StateMachineDefaultEmptyData in_play;
    StateMachineDefaultEmptyData out_of_play;
    StateMachineDefaultEmptyData gk_hands;
} StateMachineBallStateData;

DefineStateMachine(Ball);
DefineStateMachineFunctions(Ball, ball);

struct Entity *entities_ball_create(BallConfig config, GameState *game_state);
void entities_ball_set_position_and_velocity(EntityId ball_entity_id, v3 pos, v3 velocity,
                                             GameState *game_state);
void entities_ball_control_precision(EntityId ball_entity_id, EntityId player_on_the_ball_entity_id,
                                     v2 player_velocity, v2 position_to_seek, f32 control_quality,
                                     GameState *game_state);
void entities_ball_control_running(EntityId ball_entity_id, EntityId player_on_the_ball_entity_id,
                                   v2 control_direction, v2 player_velocity, f32 control_quality,
                                   GameState *game_state);
void entities_ball_shoot(EntityId ball_entity_id, EntityId kicker_entity_id, v2 direction,
                         f32 power, GameState *game_state);
void entities_ball_short_pass(EntityId ball_entity_id, EntityId kicker_entity_id, v3 direction,
                              f32 power, b32 auto_pass, GameState *game_state);
void entities_ball_long_pass(EntityId ball_entity_id, EntityId kicker_entity_id, v2 direction,
                             f32 power, GameState *game_state);
void entities_ball_held_by_goalie(EntityId ball_entity_id, EntityId goalie_entity_id,
                                  GameState *game_state);
void entities_ball_saved_by_goalie(EntityId ball_entity_id, EntityId goalie_entity_id,
                                   GameState *game_state);
void entities_ball_back_in_play(EntityId ball_entity_id, GameState *game_state);
void entities_ball_in_goal(EntityId ball_entity_id, GameState *game_state);

// END - Ball

// START - Player

typedef enum { PS_NONE, PS_NORMAL, PS_GOALIE, PS_COUNT } StateMachinePlayerState;

static char const *StateMachinePlayerStateString[] = {"none", "normal", "goalie"};

DeclareStateMachineEnterAndExit(player);

typedef struct {
    Direction running_action_direction;
} StateMachinePlayerNormalData;

typedef union {
    StateMachinePlayerNormalData normal;
    StateMachineDefaultEmptyData goalie;
} StateMachinePlayerStateData;

DefineStateMachine(Player);
DefineStateMachineFunctions(Player, player);

typedef enum { GS_NONE, GS_PROTECT_GOAL, GS_DIVE, GS_HOLD_BALL, GS_COUNT } StateMachineGoalieState;

static char const *StateMachineGoalieStateString[] = {"none", "protect_goal", "dive", "hold_ball"};

DeclareStateMachineEnterAndExit(goalie);

typedef struct {
    Time hold_start_time;
} StateMachineGoalieHoldBallData;

typedef struct {
    f64 animation_start_time;
    f64 animation_first_phase_end_time;
    f64 animation_end_time;
    b32 animation_first_phase_completed;
    b32 animation_second_phase_started;
    b32 animation_completed;
    b32 ball_saved;
    b32 ball_held;
} StateMachineGoalieDiveData;

typedef union {
    StateMachineDefaultEmptyData protect_goal;
    StateMachineGoalieDiveData dive;
    StateMachineGoalieHoldBallData hold_ball;
} StateMachineGoalieStateData;

DefineStateMachine(Goalie);
DefineStateMachineFunctions(Goalie, goalie);

struct Entity *entities_player_create(PlayerConfig config, GameState *game_state);
void entities_player_apply_force(EntityId player_entity_id, Vector2 steering,
                                 GameState *game_state);
void entities_player_apply_velocity(EntityId player_entity_id, GameState *game_state);
void entities_player_calculate_running_frame(EntityId player_entity_id, v2 direction,
                                             f32 velocity_threshold, GameState *game_state);

// END - Player

// START - Goal

struct Entity *entities_goal_create(GoalConfig config, GameState *game_state);

// END - Goal

// START - Goal net

struct Entity *entities_goal_net_create(GameState *game_state);

// END - Goal net

typedef enum { ET_CAMERA, ET_BALL, ET_PLAYER, ET_GOAL, ET_GOAL_NET, ET_COUNT } EntityType;
static const char *EntityTypeToString[ET_COUNT] = {"camera", "ball", "player", "goal", "goal_net"};

typedef struct EntityHeader {
    b32 alive;
    EntityId id;
    char name[MaxStringSize];
    EntityType type;
} EntityHeader;

typedef struct Entity {
    EntityHeader header;
    ComponentCamera camera;
    ComponentTransform transform;
    ComponentPhysics physics;
    ComponentPhysics3d physics_3d;
    ComponentSprite sprite;
    ComponentCollider collider;
    ComponentAction action;
    ComponentTarget target;
    ComponentPlayer player;
    ComponentBall ball;
    ComponentGoal goal;
    ComponentGoalie goalie;
} Entity;

typedef struct EntityRepository {
    b32 initialised;
    MemoryArenaHandle *arena_handle;
    Entity *entities;
    u32 available_slots;
    u32 max_slots;
    u32 last_used_slot;
} EntityRepository;

EntityRepository *entities_repository_init(MemoryArenaHandle *memory_arena_handle);
void entities_repository_destroy(EntityRepository *handle);

Entity *entities_entity_init(EntityRepository *handle, EntityType type, char *name_format);
void entities_entity_destroy(EntityRepository *handle, EntityId id);
Entity *entities_entity_get(EntityRepository *handle, EntityId id);
void entities_entity_update(struct Entity *, GameState *game_state, Input *input);

#endif
