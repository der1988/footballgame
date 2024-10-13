#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#define define_state_machine(ENTITY_TYPE)                                                          \
    struct StateMachine##ENTITY_TYPE                                                               \
    {                                                                                              \
	entity_id_t owner_entity;                                                                  \
	enum StateMachine##ENTITY_TYPE##State current_state;                                       \
	enum StateMachine##ENTITY_TYPE##State previous_state;                                      \
	union StateMachine##ENTITY_TYPE##StateData state_data;                                     \
    };

#define define_state_machine_functions(ENTITY_TYPE, NAME)                                          \
    void entities_##NAME##_state_machine_change_state_to(                                          \
	struct StateMachine##ENTITY_TYPE *state_machine,                                           \
	enum StateMachine##ENTITY_TYPE##State state, struct game_state *game_state);               \
    void entities_##NAME##_state_machine_on_enter(struct Entity *entity,                           \
						  struct game_state *game_state);                  \
    void entities_##NAME##_state_machine_on_exit(struct Entity *entity,                            \
						 struct game_state *game_state);

#define declare_state_machine_enter_and_exit(ENTITY_NAME)                                          \
    void state_machine_##ENTITY_NAME##_on_enter(struct Entity *ENTITY_NAME,                        \
						struct game_state *game_state);                    \
    void state_machine_##ENTITY_NAME##_on_exit(struct Entity *ENTITY_NAME,                         \
					       struct game_state *game_state);

#define implement_state_machine_functions(ENTITY_TYPE, NAME)                                       \
    void entities_##NAME##_state_machine_change_state_to(                                          \
	struct StateMachine##ENTITY_TYPE *state_machine,                                           \
	enum StateMachine##ENTITY_TYPE##State state, struct game_state *game_state)                \
    {                                                                                              \
	struct Entity *entity =                                                                    \
	    entities_entity_get(game_state->entity_repository, state_machine->owner_entity);       \
	if (state_machine->current_state == state)                                                 \
	{                                                                                          \
	    GLOGF_WARN_CTX(LS_ENTITIES, entity->header.name, "current state is already %s",        \
			   StateMachine##ENTITY_TYPE##StateString[state]);                         \
	    return;                                                                                \
	}                                                                                          \
	state_machine->previous_state = state_machine->current_state;                              \
	state_machine->current_state = state;                                                      \
	entities_##NAME##_state_machine_on_exit(entity, game_state);                               \
	entities_##NAME##_state_machine_on_enter(entity, game_state);                              \
	GLOGF_INFO_CTX(LS_ENTITIES, entity->header.name, "current state is now: %s",               \
		       StateMachine##ENTITY_TYPE##StateString[state]);                             \
    }

struct StateMachineDefaultEmptyData
{
};

#endif // !STATE_MACHINE_H
