#ifndef GAMEPLAY_H
#define GAMEPLAY_H

#include "football.h"
#include "types.h"

// Forward declaration
struct game_state;

enum gameplay_mode {
	GPM_MATCH,
	GPM_SANDBOX,
	GPM_COUNT
};

struct sandbox {
	f32 restart_delay;
};

// The mode-agnostic knobs shared code may read without knowing the mode. Each
// mode's setup publishes its own value here, so the tick never branches on the
// mode to find a number.
struct gameplay_config {
	f32 goal_celebration_time;
};

// The running gameplay session: what mode is being played, the machine driving
// it, and that mode's own data. Held by value on game_state and copied into
// every time capsule, so everything here rewinds.
struct gameplay {
	// Tag for the union below and the key the vtable is resolved from. Written
	// once by init and never again.
	enum gameplay_mode mode;
	struct gameplay_config config;
	struct state_machine_football state_machine;
	union gameplay_data {
		struct match match;
		struct sandbox sandbox;
	} data;
};

// Behaviour only: how a session is driven. Per-mode numbers are data and live
// on struct gameplay instead.
struct gameplay_vtable {
	const char *name;
	void (*setup)(struct game_state *game_state);
	state_machine_football_transition_row_t *transitions;
};

// All of these operate on game_state->gameplay: there is exactly one, held by
// value, so passing it alongside game_state would only allow the two to
// disagree.
void gameplay_init(enum gameplay_mode mode, struct game_state *game_state);
void gameplay_update(struct game_state *game_state);
void gameplay_fire(enum state_machine_football_trigger trigger, struct game_state *game_state);

// The spawn work every mode shares, differing only in how many outfielders each
// team gets.
void gameplay_setup_common(u8 outfield_players_per_team, struct game_state *game_state);

#endif // !GAMEPLAY_H
