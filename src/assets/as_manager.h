#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "memory/memory.h"

struct assets asset_manager_load_everything(memory_arena_handle_t *memory_arena);
void asset_manager_destroy_everything(struct assets assets);

#endif // !ASSET_MANAGER_H
