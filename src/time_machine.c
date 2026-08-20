#include "macro.h"
#include "entity.h"
#include "game.h"
#include "log.h"
#include "math_ext.h"
#include "memory.h"
#include "time_machine.h"
#include <raymath.h>
#include <stdio.h>

struct time_distance {
	usize since_first_recorded_position;
	usize to_last_recorded_position;
};

struct time_machine *time_machine_create(memory_arena_handle_t *memory_arena)
{
	ASSERT(memory_arena);

	struct time_machine *handle = memory_arena_push(memory_arena, sizeof(struct time_machine));
	ASSERT(handle);
	handle->arena_handle = memory_arena;
	// Allocate space for 60 time capsules per second for 30 seconds
	// It's basically a ring buffer
	handle->time_capsules =
		memory_arena_push(memory_arena, sizeof(struct time_capsule) * TIME_CAPSULES_MAX);
	ASSERT(handle->time_capsules);
	handle->last_recorded_pos = 0;
	handle->current_pos = 0;
	handle->initialised = true;

	GLOG_INFO(LS_TIME_MACHINE, "time machine initialised");

	return handle;
}

void time_machine_destroy(struct time_machine *handle)
{
	ASSERT(handle);

	if (handle->initialised) {
		handle->initialised = false;

		GLOG_INFO(LS_TIME_MACHINE, "time machine destroyed");
	}
}

static struct time_distance get_time_distance(usize last_recorded_pos,
											  usize current_pos,
											  usize recorded)
{
	usize distance_from_last_recorded_pos = 0;
	if (current_pos < last_recorded_pos) {
		distance_from_last_recorded_pos = last_recorded_pos - current_pos;
	} else if (current_pos > last_recorded_pos) {
		distance_from_last_recorded_pos = TIME_CAPSULES_MAX - current_pos + last_recorded_pos;
	}
	usize distance_from_first_recorded_pos = recorded - distance_from_last_recorded_pos - 1;

	return (struct time_distance){.since_first_recorded_position = distance_from_first_recorded_pos,
								  .to_last_recorded_position = distance_from_last_recorded_pos};
}

void time_machine_save_capsule(struct time_machine *handle, struct game_state *game_state)
{
	ASSERT(handle);

	if (++handle->last_recorded_pos == TIME_CAPSULES_MAX) {
		handle->last_recorded_pos = 0;
	}
	handle->current_pos = handle->last_recorded_pos;

	struct time_capsule *time_capsule = &handle->time_capsules[handle->last_recorded_pos];
	time_capsule->current_time = game_state->current_time;
	time_capsule->time = game_state->time;
	time_capsule->accumulator = game_state->accumulator;
	time_capsule->frame_updates = game_state->frame_updates;
	time_capsule->physics_updates = game_state->physics_updates;
	time_capsule->camera = game_state->camera;
	time_capsule->virtual_camera = game_state->virtual_camera;
	time_capsule->vars = game_state->vars;
	time_capsule->gameplay = game_state->gameplay;
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
	ASSERT(handle);

	// printf("recorded: %lu - last recorded pos %lu - current pos %lu - restoring %lu\n",
	// 	   handle->recorded,
	// 	   handle->last_recorded_pos,
	// 	   handle->current_pos,
	// 	   pos);

	struct time_capsule *time_capsule = &handle->time_capsules[pos];
	game_state->current_time = time_capsule->current_time;
	game_state->time = time_capsule->time;
	game_state->accumulator = time_capsule->accumulator;
	game_state->frame_updates = time_capsule->frame_updates;
	game_state->physics_updates = time_capsule->physics_updates;
	game_state->camera = time_capsule->camera;
	game_state->virtual_camera = time_capsule->virtual_camera;
	game_state->vars = time_capsule->vars;
	game_state->gameplay = time_capsule->gameplay;
	// game_state->constants = time_capsule->constants;
	game_state->human_players[0] = time_capsule->human_players[0];
	game_state->human_players[1] = time_capsule->human_players[1];

	// Entities
	game_state->entity_repository->available_slots = time_capsule->entities_available_slots;
	game_state->entity_repository->last_used_slot = time_capsule->entities_last_used_slot;
	for (usize i = 0; i < MAX_ENTITIES; ++i) {
		game_state->entity_repository->entities[i] = time_capsule->entities[i];
	}

	// We need to stay in recorder mode
	game_state->vars.mode = GM_VCR;

	handle->current_pos = pos;
}

void time_machine_travel(struct time_machine *handle, size delta, struct game_state *game_state)
{
	ASSERT(handle);

	if (delta == 0)
		return;

	struct time_distance time_distance =
		get_time_distance(handle->last_recorded_pos, handle->current_pos, handle->recorded);

	// printf("time distance - to_last: %lu, since_first: %lu\n",
	// 	   time_distance.to_last_recorded_position,
	// 	   time_distance.since_first_recorded_position);

	if (delta > 0) {
		if ((usize)delta > time_distance.to_last_recorded_position) {
			delta = time_distance.to_last_recorded_position;
		}

		handle->current_pos += delta;
		if (handle->current_pos >= TIME_CAPSULES_MAX)
			handle->current_pos = handle->current_pos - TIME_CAPSULES_MAX;
	} else {
		if ((usize)-delta > time_distance.since_first_recorded_position)
			delta = -time_distance.since_first_recorded_position;

		if ((size)(handle->current_pos + delta) < 0) {
			handle->current_pos = TIME_CAPSULES_MAX + (size)(handle->current_pos + delta);
		} else
			handle->current_pos += delta;
	}

	// printf("restoring to pos: %lu\n", handle->current_pos);

	time_machine_restore_capsule(handle, handle->current_pos, game_state);
}

usize time_machine_current_pos(struct time_machine *handle)
{
	ASSERT(handle);

	struct time_distance time_distance =
		get_time_distance(handle->last_recorded_pos, handle->current_pos, handle->recorded);

	return time_distance.since_first_recorded_position;
}

void time_machine_set_resume_position(struct time_machine *handle,
									  enum resume_position resume_position,
									  struct game_state *game_state)
{
	ASSERT(handle);

	switch (resume_position) {
	case RP_CURRENT: {
		struct time_distance time_distance =
			get_time_distance(handle->last_recorded_pos, handle->current_pos, handle->recorded);

		handle->recorded -= time_distance.to_last_recorded_position;

		handle->last_recorded_pos = handle->current_pos;
		break;
	}
	case RP_LAST_RECORDED:
		time_machine_restore_capsule(handle, handle->last_recorded_pos, game_state);
		break;
	}
}
