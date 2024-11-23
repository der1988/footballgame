#ifndef ASSETS_H
#define ASSETS_H

#include "memory.h"
#include "types.h"
#include <raylib.h>

#define MaxStringSize 50

// Texture Cache
typedef u32 assets_texture_id_t;

typedef struct assets_texture_cache assets_texture_cache_t;

struct assets_texture {
    assets_texture_id_t id;
    char name[MaxStringSize];
    texture_t texture;
};

struct assets_texture_cache *assets_texture_cache_create(memory_arena_handle_t *memory_arena_handle);
void assets_texture_cache_destroy(assets_texture_cache_t *handle);
struct assets_texture *assets_texture_load(assets_texture_cache_t *handle, const char *filename);
struct assets_texture *assets_texture_get_by_id(assets_texture_cache_t *handle, const u32 id);
struct assets_texture *assets_texture_get_by_name(assets_texture_cache_t *handle,
						  const char *filename);

// Sprite Frame Cache
typedef u32 assets_sprite_frame_id_t;

typedef struct assets_sprite_frame_cache assets_sprite_frame_cache_t;

struct assets_sprite_frame {
    assets_sprite_frame_id_t id;
    char name[MaxStringSize];
    rect_t frame;
    b32 rotated;
    b32 trimmed;
    rect_t sprite_source_size;
    v2 source_size;
    v2 pivot;
    u32 texture_id;
};

assets_sprite_frame_cache_t *
assets_sprite_frame_cache_create(memory_arena_handle_t *memory_arena_handle);
void assets_sprite_frame_cache_destroy(assets_sprite_frame_cache_t *handle);
i32 assets_sprite_frames_load_from_json(assets_sprite_frame_cache_t *handle,
					const u32 texture_id,
					const char *filename);
struct assets_sprite_frame *assets_sprite_frame_get_by_name(assets_sprite_frame_cache_t *handle,
							    const char *name);
struct assets_sprite_frame *assets_sprite_frame_get_by_id(assets_sprite_frame_cache_t *handle,
							  const assets_sprite_frame_id_t id);

// Texture Packer Helper
struct assets_sprite_frame *assets_texture_packer_load_from_json(
    const char *filename, const u32 texture_id, const usize initial_sprite_frame_id, usize *count);

// Animation Cache
typedef u32 assets_animation_id_t;

struct assets_animation {
    assets_animation_id_t id;
    char name[MaxStringSize];
    f32 delay;
    assets_sprite_frame_id_t *frames;
    u16 frames_count;
};

typedef struct assets_animation_cache assets_animation_cache_t;

assets_animation_cache_t *assets_animation_cache_create(memory_arena_handle_t *memory_arena_handle);
void assets_animation_cache_destroy(assets_animation_cache_t *handle);
i32 assets_animations_load_from_json(assets_animation_cache_t *handle,
				     assets_sprite_frame_cache_t *sprite_frame_cache,
				     const char *filename);
struct assets_animation *assets_animation_get_by_name(assets_animation_cache_t *handle,
						      const char *name);
struct assets_animation *assets_animation_get_by_id(assets_animation_cache_t *handle,
						    const assets_animation_id_t id);

// Shader Cache
typedef u32 assets_shader_id_t;

typedef struct assets_shader_cache assets_shader_cache_t;

struct assets_shader {
    assets_shader_id_t id;
    char name[MaxStringSize];
    char vs_filename[MaxStringSize];
    char fs_filename[MaxStringSize];
    shader_t shader;
};

assets_shader_cache_t *assets_shader_cache_create(memory_arena_handle_t *memory_arena_handle);
struct assets_shader *assets_shader_load(assets_shader_cache_t *handle,
					 const char *name,
					 const char *vs_filename,
					 const char *fs_filename);
void assets_shader_cache_destroy(assets_shader_cache_t *handle);
struct assets_shader *assets_shader_get_by_id(assets_shader_cache_t *handle, u32 id);
struct assets_shader *assets_shader_get_by_name(assets_shader_cache_t *handle, const char *name);

struct assets {
    assets_texture_cache_t *texture_cache;
    assets_sprite_frame_cache_t *sprite_frame_cache;
    assets_animation_cache_t *animation_cache;
    assets_shader_cache_t *shader_cache;
};

struct assets assets_load_everything(memory_arena_handle_t *memory_arena);
void assets_destroy_everything(struct assets assets);

#endif
