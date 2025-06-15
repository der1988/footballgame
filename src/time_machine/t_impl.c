#include "memory.h"
#include "entities.h"
#include "time_machine.h"

struct time_machine *time_machine_create(memory_arena_handle_t *memory_arena)
{
	struct time_machine *handle = memory_arena_push(memory_arena, sizeof(struct time_machine));
	handle->time_capsules =
		memory_arena_push(memory_arena, sizeof(struct time_capsule) * 30 * MAX_ENTITIES);

	return handle;
}

void time_machine_destroy(struct time_machine *time_machine);
void time_machine_save_capsule(struct time_machine *time_machine, struct game_state *game_state);
void time_machine_restore_capsule(struct time_machine *time_machine,
								  usize pos,
								  struct game_state *game_state);
void time_machine_rewind(struct time_machine *time_machine,
						 u64 seconds,
						 struct game_state *game_state);
usize time_machine_current_pos();
