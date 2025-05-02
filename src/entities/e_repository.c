#include "entities.h"
#include "log.h"
#include <assert.h>
#include <stdio.h>

static entity_id_t find_empty_slot(struct entity_repository *entity_repo)
{
	// TODO(marco): is a linear search enough here?
	entity_id_t id = 0;
	if (entity_repo->available_slots == entity_repo->max_slots) {
		entity_repo->available_slots--;
		entity_repo->last_used_slot = id;
		entity_repo->entities[id].header.id = id;
		entity_repo->entities[id].header.alive = true;
	} else if (entity_repo->available_slots > 0) {
		id = entity_repo->last_used_slot;
		do {
			if (id == entity_repo->max_slots) {
				id = 0;
			} else {
				id++;
			}
		} while ((entity_repo->entities[id].header.alive));
		entity_repo->available_slots--;
		entity_repo->last_used_slot = id;
		entity_repo->entities[id].header.id = id;
		entity_repo->entities[id].header.alive = true;
	} else {
		GLOG_ERROR(LS_ENTITIES, "No more entity slots available");
		return EID_NOT_VALID;
	}

	return id;
}

static void free_slot(struct entity_repository *entity_repo, entity_id_t id)
{
	entity_repo->entities[id].header.alive = false;
	entity_repo->available_slots++;
}

struct entity_repository *entities_repository_init(memory_arena_handle_t *memory_arena_handle)
{
	struct entity_repository *handle =
		memory_arena_push(memory_arena_handle, sizeof(struct entity_repository));
	assert(handle);

	handle->arena_handle = memory_arena_handle;
	handle->entities =
		memory_arena_push(handle->arena_handle, sizeof(struct entity) * MAX_ENTITIES);
	handle->max_slots = handle->available_slots = MAX_ENTITIES;
	handle->last_used_slot = 0;
	handle->initialised = true;
	GLOG_INFO(LS_ENTITIES, "entities repository initialised");
	return handle;
}

void entities_repository_destroy(struct entity_repository *handle)
{
	assert(handle->initialised);
	handle->initialised = false;
	GLOG_INFO(LS_ENTITIES, "entities repository destroyed");
}

struct entity *entity_init(struct entity_repository *handle,
						   enum entity_type type,
						   char *name_format)
{
	assert(handle->initialised && handle->available_slots > 0);

	entity_id_t id = find_empty_slot(handle);
	if (id == EID_NOT_VALID) {
		return NULL;
	}

	struct entity *entity = &handle->entities[id];

	// Entity header
	entity->header.type = type;
	snprintf(entity->header.name, 40, name_format, entity->header.id);

	GLOG_INFO_CTX(LS_ENTITIES, entity->header.name, "entity initialised");

	return entity;
}

void entity_destroy(struct entity_repository *handle, entity_id_t id)
{
	free_slot(handle, id);

	GLOG_INFO_CTX(LS_ENTITIES, handle->entities[id].header.name, "entity destroyed");
}

struct entity *entity_get(struct entity_repository *handle, entity_id_t id)
{
	assert(handle->initialised);
	return id >= 0 && id < handle->max_slots ? &handle->entities[id] : NULL;
}

b32 entity_is_alive_and_has_components(struct entity *entity, u32 count, ...)
{
	if (!entity->header.alive) {
		return false;
	}
	b32 valid = true;
	va_list ap;
	va_start(ap, count);
	for (u32 i = 0; i < count; ++i) {
		struct component_header *component =
			(struct component_header *)(((void *)entity) + va_arg(ap, usize));
		if (!component->enabled) {
			valid = false;
			break;
		}
	}
	va_end(ap);
	return valid;
}
