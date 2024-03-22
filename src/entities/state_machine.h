#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#define DefineStateMachine(ENTITY_TYPE, N_STATES)                                                  \
    typedef struct StateMachine##ENTITY_TYPE {                                                     \
        EntityId owner_entity;                                                                     \
        StateMachine##ENTITY_TYPE##State current_state;                                            \
        StateMachine##ENTITY_TYPE##State previous_state;                                           \
        StateMachine##ENTITY_TYPE##StateData state_data;                                           \
    } StateMachine##ENTITY_TYPE;

#define ImplementStateMachineFunctions(ENTITY_TYPE, NAME)                                          \
    static void state_machine_##NAME##_change_state_to(StateMachine##ENTITY_TYPE *state_machine,   \
                                                       StateMachine##ENTITY_TYPE##State state,     \
                                                       GameState *game_state) {                    \
        Entity *entity =                                                                           \
            entities_entity_get(game_state->entity_repository, state_machine->owner_entity);       \
        if (state_machine->current_state == state) {                                               \
            GLOGF_WARN_CTX(LS_ENTITIES, entity->name, "current state is already %s",               \
                           StateMachine##ENTITY_TYPE##StateString[state]);                         \
            return;                                                                                \
        }                                                                                          \
        state_machine->previous_state = state_machine->current_state;                              \
        state_machine->current_state = state;                                                      \
        state_machine_##NAME##_on_exit(entity, game_state);                                        \
        state_machine_##NAME##_on_enter(entity, game_state);                                       \
        GLOGF_INFO_CTX(LS_ENTITIES, entity->name, "current state is now: %s",                      \
                       StateMachine##ENTITY_TYPE##StateString[state]);                             \
    }

typedef struct {
} StateMachineDefaultEmptyData;

#endif // !STATE_MACHINE_H
