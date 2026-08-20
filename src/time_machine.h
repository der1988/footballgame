#ifndef TIME_MACHINE_H
#define TIME_MACHINE_H

#include "entity.h"
#include "game.h"
#include "memory.h"

// we record 30 time capsules every second for 30 seconds
#define TIME_MACHINE_MAX_REWIND_TIME 30
#define TIME_CAPSULES_PER_SECOND	 480
#define TIME_CAPSULES_MAX			 (TIME_MACHINE_MAX_REWIND_TIME * TIME_CAPSULES_PER_SECOND)

struct time_capsule {
	fg_time_t current_time;
	f64 accumulator;
	fg_time_t time;
	u64 physics_updates;
	u64 frame_updates;
	struct game_constants constants;
	struct game_vars vars;
	struct gameplay gameplay;
	struct human_player human_players[MAX_HUMAN_PLAYERS];
	struct entity_repository entity_repository;
	camera2d_t camera;
	camera2d_t virtual_camera;

	// Entities
	struct entity entities[MAX_ENTITIES];
	u32 entities_available_slots;
	u32 entities_last_used_slot;
};

struct time_machine {
	b32 initialised;
	memory_arena_handle_t *arena_handle;
	struct time_capsule *time_capsules;
	usize last_recorded_pos;
	usize current_pos;
	usize recorded;
};

enum resume_position {
	RP_CURRENT,
	RP_LAST_RECORDED
};

struct time_machine *time_machine_create(memory_arena_handle_t *memory_arena);
void time_machine_destroy(struct time_machine *handle);
void time_machine_save_capsule(struct time_machine *handle, struct game_state *game_state);
void time_machine_restore_capsule(struct time_machine *handle,
								  usize pos,
								  struct game_state *game_state);
void time_machine_travel(struct time_machine *handle, size delta, struct game_state *game_state);
usize time_machine_current_pos(struct time_machine *handle);
void time_machine_set_resume_position(struct time_machine *handle,
									  enum resume_position resume_position,
									  struct game_state *game_state);

#endif // !TIME_MACHINE_H
