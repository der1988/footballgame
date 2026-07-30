#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#define define_state_machine(TYPE, CTX_TYPE)                                                       \
	struct state_machine_##TYPE {                                                                  \
		CTX_TYPE *ctx;                                                                             \
		enum state_machine_##TYPE##_state current_state;                                           \
		enum state_machine_##TYPE##_state previous_state;                                          \
		union state_machine_##TYPE##_state_data state_data;                                        \
	};

#define define_state_machine_functions(TYPE, CTX_TYPE)                                             \
	void TYPE##_state_machine_change_state_to(struct state_machine_##TYPE *state_machine,          \
											  enum state_machine_##TYPE##_state state,             \
											  struct game_state *game_state);

#define forward_declare_state_machine_functions(TYPE, CTX_TYPE)                                    \
	static void TYPE##_state_machine_on_enter(CTX_TYPE *ctx, struct game_state *game_state);        \
	static void TYPE##_state_machine_on_exit(CTX_TYPE *ctx, struct game_state *game_state);

#define implement_state_machine_functions(TYPE, CTX_TYPE)                                          \
	void TYPE##_state_machine_change_state_to(struct state_machine_##TYPE *state_machine,          \
											  enum state_machine_##TYPE##_state state,             \
											  struct game_state *game_state)                       \
	{                                                                                              \
		if (state_machine->current_state == state) {                                              \
			GLOGF_WARN_CTX(LS_STATE_MACHINE,                                                       \
						   #TYPE,                                                                  \
						   "state is already %s",                                                 \
						   state_machine_##TYPE##_state_string[state]);                           \
			return;                                                                                \
		}                                                                                          \
		state_machine->previous_state = state_machine->current_state;                             \
		state_machine->current_state = state;                                                     \
		TYPE##_state_machine_on_exit(state_machine->ctx, game_state);                              \
		TYPE##_state_machine_on_enter(state_machine->ctx, game_state);                             \
	}

struct state_machine_default_empty_data {};

#endif // !STATE_MACHINE_H
