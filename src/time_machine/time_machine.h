#include "entities.h"
#include "memory.h"

struct time_capsule {
	time_t time;
	struct entity entities[MAX_ENTITIES];
};

struct time_machine {
	struct time_capsule *time_capsules;
	usize current_pos;
};

struct time_machine *time_machine_create(memory_arena_handle_t *memory_arena);
void time_machine_destroy(struct time_machine *time_machine);
void time_machine_save_capsule(struct time_machine *time_machine, struct game_state *game_state);
void time_machine_restore_capsule(struct time_machine *time_machine,
								  usize pos,
								  struct game_state *game_state);
void time_machine_rewind(struct time_machine *time_machine,
						 u64 seconds,
						 struct game_state *game_state);
usize time_machine_current_pos();
