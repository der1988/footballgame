#include "../components/components.h"
#include "../entities/entities.h"
#include "../memory/memory.h"
#include "../utils/utils.h"
#include "memory.h"
#include "types.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

static EntityId find_empty_slot(EntityRepository *entity_repo) {
    // TODO(marco): is a linear search enough here?
    EntityId id = 0;
    if (entity_repo->available_slots == entity_repo->max_slots) {
        entity_repo->available_slots--;
        entity_repo->last_used_slot = id;
        entity_repo->entities[id].id = id;
        entity_repo->entities[id].alive = true;
    } else if (entity_repo->available_slots > 0) {
        id = entity_repo->last_used_slot;
        do {
            if (id == entity_repo->max_slots) {
                id = 0;
            } else {
                id++;
            }
        } while ((entity_repo->entities[id].alive));
        entity_repo->available_slots--;
        entity_repo->last_used_slot = id;
        entity_repo->entities[id].id = id;
        entity_repo->entities[id].alive = true;
    } else {
        GLOG_ERROR(LS_ENTITIES, "No more entities available");
    }

    return id;
}

static void empty_slot(EntityRepository *entity_repo, EntityId id) {
    entity_repo->entities[id].alive = false;
    entity_repo->available_slots++;
}

EntityRepository *entities_repository_init(MemoryArenaHandle *memory_arena_handle) {
    EntityRepository *handle = memory_arena_push(memory_arena_handle, sizeof(EntityRepository));
    assert(handle);

    handle->arena_handle = memory_arena_handle;
    handle->entities = memory_arena_push(handle->arena_handle, sizeof(Entity) * MAX_ENTITIES);
    handle->components =
        memory_arena_push(handle->arena_handle, CT_COUNT * sizeof(Component) * MAX_ENTITIES);
    handle->max_slots = handle->available_slots = MAX_ENTITIES;
    handle->last_used_slot = 0;
    handle->initialised = true;
    GLOG_INFO(LS_ENTITIES, "entities repository initialised");
    return handle;
}

void entities_repository_destroy(EntityRepository *handle) {
    assert(handle->initialised);
    handle->initialised = false;
    GLOG_INFO(LS_ENTITIES, "entities repository destroyed");
}

Entity *entities_entity_init(EntityRepository *handle, EntityType type, char *name_format) {
    assert(handle->initialised && handle->available_slots > 0);

    EntityId id = find_empty_slot(handle);
    Entity *entity = &handle->entities[id];

    // Entity header
    entity->type = type;
    snprintf(entity->name, 40, name_format, entity->id);

    GLOG_INFO_CTX(LS_ENTITIES, entity->name, "entity initialised");

    return entity;
}

void entities_entity_destroy(EntityRepository *handle, EntityId id) {
    empty_slot(handle, id);

    GLOG_INFO_CTX(LS_ENTITIES, handle->entities[id].name, "entity destroyed");
}

Entity *entities_entity_get(EntityRepository *handle, EntityId id) {
    assert(handle->initialised);
    return handle->entities[id].alive ? &handle->entities[id] : NULL;
}

void *entities_component_create(EntityRepository *entity_repo, ComponentType type, EntityId id) {
    assert(entity_repo->initialised);

    Component *component = entity_repo->components + ((type * MAX_ENTITIES) + id);
    component->header.type = type;
    component->header.enabled = true;
    component->header.entity_id = id;

    // We return a random union field, since they all start at the same address in memory
    return &component->transform;
}

Component *entities_component_get(EntityRepository *entity_repo, ComponentType type, EntityId id) {
    assert(entity_repo->initialised);

    if (!entity_repo->entities[id].alive) {
        return NULL;
    }

    Component *component = entity_repo->components + ((type * entity_repo->max_slots) + id);

    return component;
}

void *entities_component_get_typed(EntityRepository *entity_repo, ComponentType type, EntityId id) {
    assert(entity_repo->initialised);

    if (!entity_repo->entities[id].alive) {
        return NULL;
    }

    Component *component = entity_repo->components + ((type * entity_repo->max_slots) + id);
    assert(component->header.enabled);

    return &component->transform;
}

b32 entities_entity_is_alive_and_has_components(EntityRepository *entity_repo, EntityId id,
                                                u32 count, ...) {
    if (!entity_repo->entities[id].alive) {
        return false;
    }

    va_list ap;
    va_start(ap, count);
    for (usize i = 0; i < count; ++i) {
        ComponentType type = va_arg(ap, ComponentType);
        Component *component = entity_repo->components + ((type * entity_repo->max_slots) + id);
        if (!component->header.enabled) {
            return false;
        }
    };
    va_end(ap);

    return true;
}
