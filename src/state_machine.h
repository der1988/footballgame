#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#define STATE_MACHINE_DEFINE(TYPE, CTX_TYPE)                                                       \
	struct state_machine_##TYPE {                                                                  \
		CTX_TYPE *ctx;                                                                             \
		enum state_machine_##TYPE##_state current_state;                                           \
		enum state_machine_##TYPE##_state previous_state;                                          \
		union state_machine_##TYPE##_state_data state_data;                                        \
	};

#define STATE_MACHINE_DECLARE_FUNCTIONS(TYPE, CTX_TYPE)                                             \
	void state_machine_##TYPE##_change_state_to(struct state_machine_##TYPE *state_machine,          \
											  enum state_machine_##TYPE##_state state,             \
											  struct game_state *game_state);

#define STATE_MACHINE_FORWARD_DECLARE_FUNCTIONS(TYPE, CTX_TYPE)                                    \
	static void state_machine_##TYPE##_on_enter(CTX_TYPE *ctx, struct game_state *game_state);        \
	static void state_machine_##TYPE##_on_exit(CTX_TYPE *ctx, struct game_state *game_state);

#define STATE_MACHINE_IMPLEMENT_FUNCTIONS(TYPE, CTX_TYPE)                                          \
	void state_machine_##TYPE##_change_state_to(struct state_machine_##TYPE *state_machine,          \
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
		state_machine_##TYPE##_on_exit(state_machine->ctx, game_state);                              \
		state_machine_##TYPE##_on_enter(state_machine->ctx, game_state);                             \
	}

struct state_machine_default_empty_data {};

#endif // !STATE_MACHINE_H
