#ifndef ENTITIES_H
#define ENTITIES_H

#include "../components/components.h"
#include "../entities/state_machine.h"
#include "../input/input.h"
#include "../memory/memory.h"
#include "../utils/utils.h"
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

#define ComponentCastToInternalUnchecked(entity_repo, type, entity_id)                             \
    (void *)((char *)(entity_repo->components + (type * entity_repo->max_slots) + entity_id) +     \
             utils_memory_align(sizeof(ComponentHeader), DEFAULT_MEMORY_ALIGNMENT));

#define DeclareEntityCreateAndUpdate(ENTITY_NAME)                                                  \
    struct Entity *entities_##ENTITY_NAME##_create(GameState *game_state);                         \
    void entities_##ENTITY_NAME##_update(struct Entity *entity, GameState *game_state,             \
                                         Input *input);

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

DeclareEntityCreateAndUpdate(camera);

// END - Camera

// START - Ball

DeclareEntityCreateAndUpdate(ball);

typedef enum { BS_NONE, BS_IN_PLAY, BS_OUT_OF_PLAY, BS_COUNT } StateMachineBallState;

static char const *StateMachineBallStateString[] = {"none", "in_play", "out_of_play", "count"};

DeclareStateMachineEnterAndExit(ball);

typedef union {
    StateMachineDefaultEmptyData in_play;
    StateMachineDefaultEmptyData out_of_play;
} StateMachineBallStateData;

DefineStateMachine(Ball, BS_COUNT);
DefineStateMachineFunctions(Ball, ball);

// END - Ball

// START - Player

DeclareEntityCreateAndUpdate(player);

typedef enum { PS_NONE, PS_RUN, PS_COUNT } StateMachinePlayerState;

static char const *StateMachinePlayerStateString[] = {"none", "run", "count"};

DeclareStateMachineEnterAndExit(player);

typedef struct {
    Direction running_action_direction;
} StateMachinePlayerRunData;

typedef union {
    StateMachineDefaultEmptyData stand;
    StateMachinePlayerRunData run;
} StateMachinePlayerStateData;

DefineStateMachine(Player, PS_COUNT);
DefineStateMachineFunctions(Player, player);

void entities_player_apply_force(Vector2 steering, ComponentPhysics *physics, DeltaTime dt);
void entities_player_apply_velocity(ComponentTransform *transform, ComponentPhysics *physics,
                                    DeltaTime dt);
// END - Player

typedef enum { ET_CAMERA, ET_BALL, ET_PLAYER, ET_COUNT } EntityType;
static const char *EntityTypeToString[ET_COUNT] = {"camera", "ball", "player"};

typedef struct Entity {
    b32 alive;
    EntityId id;
    char name[40];
    EntityType type;
} Entity;

typedef struct EntityRepository {
    b32 initialised;
    MemoryArenaHandle *arena_handle;
    Entity *entities;
    Component *components;
    u32 available_slots;
    u32 max_slots;
    u32 last_used_slot;
} EntityRepository;

typedef struct EntityQueryResult {
    Entity *entities;
    usize count;
} EntityQueryResult;

typedef u64 EntityTypeQueryMask;

EntityRepository *entities_repository_init(MemoryArenaHandle *memory_arena_handle);
void entities_repository_destroy(EntityRepository *handle);

Entity *entities_entity_init(EntityRepository *handle, EntityType type, char *name_format);
void entities_entity_destroy(EntityRepository *handle, EntityId id);
Entity *entities_entity_get(EntityRepository *handle, EntityId id);
void entities_entity_update(struct Entity *, GameState *game_state, Input *input);

Component *entities_component_create(EntityRepository *handle, ComponentType type, EntityId id);
Component *entities_component_get(EntityRepository *handle, ComponentType type, EntityId id);
void *entities_component_get_internal(EntityRepository *handle, ComponentType type, EntityId id);
b32 entities_entity_is_alive_and_has_components(EntityRepository *handle, EntityId id, u32 count,
                                                ...);

#endif
