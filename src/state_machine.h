#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "log.h"

// A machine is driven by triggers, never by naming a destination state: a
// caller reports what happened and the table decides where that leads.
// transitions[FROM][TRIGGER] is the resulting state, or *_NONE when that state
// does not respond to that trigger.
//
// Every machine's state and trigger enums start with a *_NONE entry, which is
// what the machine holds before init and what an unhandled cell reads as.
//
// A machine of type TYPE must provide, alongside the macros below:
//   enum state_machine_TYPE_state, enum state_machine_TYPE_trigger
//   union state_machine_TYPE_state_data
//   const char *state_machine_TYPE_state_string[], *_trigger_string[]
#define STATE_MACHINE_DEFINE(TYPE, CTX_TYPE, TRIGGER_COUNT)                                        \
	typedef const enum state_machine_##TYPE##_state                                                \
		state_machine_##TYPE##_transition_row_t[TRIGGER_COUNT];                                    \
                                                                                                   \
	struct state_machine_##TYPE {                                                                  \
		CTX_TYPE *ctx;                                                                             \
		state_machine_##TYPE##_transition_row_t *transitions;                                      \
		enum state_machine_##TYPE##_state current_state;                                           \
		enum state_machine_##TYPE##_state previous_state;                                          \
		union state_machine_##TYPE##_state_data state_data;                                        \
	};

#define STATE_MACHINE_DECLARE_FUNCTIONS(TYPE, CTX_TYPE)                                            \
	void state_machine_##TYPE##_init(struct state_machine_##TYPE *state_machine,                    \
									 CTX_TYPE *ctx,                                                \
									 state_machine_##TYPE##_transition_row_t *transitions,         \
									 enum state_machine_##TYPE##_state initial_state,              \
									 struct game_state *game_state);                               \
	void state_machine_##TYPE##_fire(struct state_machine_##TYPE *state_machine,                    \
									 enum state_machine_##TYPE##_trigger trigger,                  \
									 struct game_state *game_state);

#define STATE_MACHINE_FORWARD_DECLARE_FUNCTIONS(TYPE, CTX_TYPE)                                    \
	static void state_machine_##TYPE##_on_enter(CTX_TYPE *ctx,                                     \
												enum state_machine_##TYPE##_state from,            \
												enum state_machine_##TYPE##_trigger trigger,       \
												enum state_machine_##TYPE##_state to,              \
												struct game_state *game_state);                    \
	static void state_machine_##TYPE##_on_exit(CTX_TYPE *ctx,                                      \
											   enum state_machine_##TYPE##_state from,             \
											   enum state_machine_##TYPE##_trigger trigger,        \
											   enum state_machine_##TYPE##_state to,               \
											   struct game_state *game_state);

#define STATE_MACHINE_IMPLEMENT_FUNCTIONS(TYPE, CTX_TYPE)                                          \
	void state_machine_##TYPE##_init(struct state_machine_##TYPE *state_machine,                    \
									 CTX_TYPE *ctx,                                                \
									 state_machine_##TYPE##_transition_row_t *transitions,         \
									 enum state_machine_##TYPE##_state initial_state,              \
									 struct game_state *game_state)                                \
	{                                                                                              \
		const enum state_machine_##TYPE##_state none = 0;                                          \
		if (!transitions || initial_state == none) {                                               \
			GLOGF_PANIC_CTX(LS_STATE_MACHINE,                                                      \
							#TYPE,                                                                 \
							"bad init: table %s, initial state %s",                                \
							transitions ? "ok" : "missing",                                        \
							state_machine_##TYPE##_state_string[initial_state]);                   \
		}                                                                                          \
		state_machine->ctx = ctx;                                                                  \
		state_machine->transitions = transitions;                                                  \
		state_machine->previous_state = none;                                                      \
		state_machine->current_state = initial_state;                                              \
		state_machine_##TYPE##_on_enter(ctx, none, 0, initial_state, game_state);                  \
	}                                                                                              \
                                                                                                   \
	void state_machine_##TYPE##_fire(struct state_machine_##TYPE *state_machine,                    \
									 enum state_machine_##TYPE##_trigger trigger,                  \
									 struct game_state *game_state)                                \
	{                                                                                              \
		const enum state_machine_##TYPE##_state from = state_machine->current_state;               \
		const enum state_machine_##TYPE##_state to = state_machine->transitions[from][trigger];    \
		/* Includes firing *_NONE: no table ever maps that column. */                              \
		if (to == 0) {                                                                             \
			GLOGF_PANIC_CTX(LS_STATE_MACHINE,                                                      \
							#TYPE,                                                                 \
							"unhandled trigger: %s in state %s",                                   \
							state_machine_##TYPE##_trigger_string[trigger],                        \
							state_machine_##TYPE##_state_string[from]);                            \
		}                                                                                          \
		state_machine_##TYPE##_on_exit(state_machine->ctx, from, trigger, to, game_state);         \
		state_machine->previous_state = from;                                                      \
		state_machine->current_state = to;                                                         \
		state_machine_##TYPE##_on_enter(state_machine->ctx, from, trigger, to, game_state);        \
	}

struct state_machine_default_empty_data {};

#endif // !STATE_MACHINE_H
