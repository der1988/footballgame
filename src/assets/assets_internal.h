#ifndef ASSETS_INTERNAL_H
#define ASSETS_INTERNAL_H

#include "memory.h"
#include "types.h"

struct assets_texture_cache
{
    b32 initialised;
    struct assets_texture *cache;
    u32 count;
};

struct assets_sprite_frame_cache
{
    b32 initialised;
    memory_arena_handle_t *memory_arena_handle;
    struct assets_sprite_frame *cache;
    u32 count;
};

struct assets_animation_cache
{
    b32 initialised;
    memory_arena_handle_t *memory_arena_handle;
    struct assets_animation *cache;
    u32 count;
};

#endif
