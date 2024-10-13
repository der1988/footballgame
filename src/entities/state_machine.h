#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#define define_state_machine(ENTITY_TYPE)                                                          \
    struct state_machine_##ENTITY_TYPE                                                             \
    {                                                                                              \
	entity_id_t owner_entity;                                                                  \
	enum state_machine_##ENTITY_TYPE##_state current_state;                                    \
	enum state_machine_##ENTITY_TYPE##_state previous_state;                                   \
	union state_machine_##ENTITY_TYPE##_state_data state_data;                                 \
    };

#define define_state_machine_functions(ENTITY_TYPE)                                                \
    void entities_##ENTITY_TYPE##_state_machine_change_state_to(                                   \
	struct state_machine_##ENTITY_TYPE *state_machine,                                         \
	enum state_machine_##ENTITY_TYPE##_state state, struct game_state *game_state);            \
    void entities_##ENTITY_TYPE##_state_machine_on_enter(struct entity *entity,                    \
							 struct game_state *game_state);           \
    void entities_##ENTITY_TYPE##_state_machine_on_exit(struct entity *entity,                     \
							struct game_state *game_state);

#define declare_state_machine_enter_and_exit(ENTITY_NAME)                                          \
    void state_machine_##ENTITY_NAME##_on_enter(struct entity *ENTITY_NAME,                        \
						struct game_state *game_state);                    \
    void state_machine_##ENTITY_NAME##_on_exit(struct entity *ENTITY_NAME,                         \
					       struct game_state *game_state);

#define implement_state_machine_functions(ENTITY_TYPE)                                             \
    void entities_##ENTITY_TYPE##_state_machine_change_state_to(                                   \
	struct state_machine_##ENTITY_TYPE *state_machine,                                         \
	enum state_machine_##ENTITY_TYPE##_state state, struct game_state *game_state)             \
    {                                                                                              \
	struct entity *entity =                                                                    \
	    entities_entity_get(game_state->entity_repository, state_machine->owner_entity);       \
	if (state_machine->current_state == state)                                                 \
	{                                                                                          \
	    GLOGF_WARN_CTX(LS_ENTITIES, entity->header.name, "current state is already %s",        \
			   state_machine_##ENTITY_TYPE##_state_string[state]);                     \
	    return;                                                                                \
	}                                                                                          \
	state_machine->previous_state = state_machine->current_state;                              \
	state_machine->current_state = state;                                                      \
	entities_##ENTITY_TYPE##_state_machine_on_exit(entity, game_state);                        \
	entities_##ENTITY_TYPE##_state_machine_on_enter(entity, game_state);                       \
	GLOGF_INFO_CTX(LS_ENTITIES, entity->header.name, "current state is now: %s",               \
		       state_machine_##ENTITY_TYPE##_state_string[state]);                         \
    }

struct state_machine_default_empty_data
{
};

#endif // !STATE_MACHINE_H
