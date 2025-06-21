#include "entities.h"
#include "log.h"
#include "math_ext.h"
#include "memory.h"
#include "time_machine.h"
#include <assert.h>
#include <raymath.h>
#include <stdio.h>

struct time_machine *time_machine_create(memory_arena_handle_t *memory_arena)
{
	assert(memory_arena);

	struct time_machine *handle = memory_arena_push(memory_arena, sizeof(struct time_machine));
	handle->arena_handle = memory_arena;
	// Allocate space for 30 time capsules per second per 30 seconds
	// It's basically a ring buffer
	handle->time_capsules =
		memory_arena_push(memory_arena, sizeof(struct time_capsule) * TIME_CAPSULES_MAX);
	handle->current_pos = 0;
	handle->initialised = true;

	GLOG_INFO(LS_TIME_MACHINE, "time machine initialised");

	return handle;
}

void time_machine_destroy(struct time_machine *handle)
{
	assert(handle);

	if (handle->initialised) {
		handle->initialised = false;

		GLOG_INFO(LS_TIME_MACHINE, "time machine destroyed");
	}
}

void time_machine_save_capsule(struct time_machine *handle, struct game_state *game_state)
{
	assert(handle);

	if (++handle->current_pos == TIME_CAPSULES_MAX) {
		handle->current_pos = 0;
	}

	struct time_capsule *time_capsule = &handle->time_capsules[handle->current_pos];
	time_capsule->current_time = game_state->current_time;
	time_capsule->time = game_state->time;
	time_capsule->accumulator = game_state->accumulator;
	time_capsule->frame_updates = game_state->frame_updates;
	time_capsule->physics_updates = game_state->physics_updates;
	time_capsule->camera = game_state->camera;
	time_capsule->virtual_camera = game_state->virtual_camera;
	time_capsule->vars = game_state->vars;
	time_capsule->constants = game_state->constants;
	time_capsule->human_players[0] = game_state->human_players[0];
	time_capsule->human_players[1] = game_state->human_players[1];

	// Entities
	time_capsule->entities_available_slots = game_state->entity_repository->available_slots;
	time_capsule->entities_last_used_slot = game_state->entity_repository->last_used_slot;
	FOR_EACH_ENTITY(entity, game_state->entity_repository)
	{
		time_capsule->entities[entity_id] = *entity;
	}

	handle->recorded = MIN(++handle->recorded, TIME_CAPSULES_MAX);
}

void time_machine_restore_capsule(struct time_machine *handle,
								  usize pos,
								  struct game_state *game_state)
{
	assert(handle);

	printf("recorded: %lu - current pos %lu - restoring %lu\n",
		   handle->recorded,
		   handle->current_pos,
		   pos);

	usize to_rewind;
	if (pos > handle->current_pos) {
		to_rewind = handle->current_pos + TIME_CAPSULES_MAX - pos;
		assert(to_rewind < handle->recorded);
	} else {
		to_rewind = handle->current_pos - pos;
	}

	struct time_capsule *time_capsule = &handle->time_capsules[pos];
	game_state->current_time = time_capsule->current_time;
	game_state->time = time_capsule->time;
	game_state->accumulator = time_capsule->accumulator;
	game_state->frame_updates = time_capsule->frame_updates;
	game_state->physics_updates = time_capsule->physics_updates;
	game_state->camera = time_capsule->camera;
	game_state->virtual_camera = time_capsule->virtual_camera;
	game_state->vars = time_capsule->vars;
	game_state->constants = time_capsule->constants;
	game_state->human_players[0] = time_capsule->human_players[0];
	game_state->human_players[1] = time_capsule->human_players[1];

	// Entities
	game_state->entity_repository->available_slots = time_capsule->entities_available_slots;
	game_state->entity_repository->last_used_slot = time_capsule->entities_last_used_slot;
	for (usize i = 0; i < MAX_ENTITIES; ++i) {
		game_state->entity_repository->entities[i] = time_capsule->entities[i];
	}

	handle->current_pos = pos;

	handle->recorded -= to_rewind;
}

void time_machine_rewind(struct time_machine *handle, u64 positions, struct game_state *game_state)
{
	assert(handle);
	assert(positions < TIME_CAPSULES_MAX);

	const usize to_rewind = positions;

	if (to_rewind < handle->recorded) {
		usize new_pos;

		if (handle->current_pos > to_rewind) {
			new_pos = handle->current_pos - to_rewind;
		} else {
			new_pos = TIME_CAPSULES_MAX - 1 - (to_rewind - handle->current_pos);
		}

		printf("restoring to pos: %lu\n", new_pos);

		time_machine_restore_capsule(handle, new_pos, game_state);
	}
}

usize time_machine_current_pos(struct time_machine *handle)
{
	assert(handle);

	return handle->current_pos;
}
