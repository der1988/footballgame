#ifndef ASSET_SHADER_H
#define ASSET_SHADER_H

#include "types.h"
#include "memory.h"

#define SHADER_NAME_MAX_SIZE	 50
#define SHADER_FILENAME_MAX_SIZE 50

// Shader Cache
typedef u32 assets_shader_id_t;

typedef struct assets_shader_cache assets_shader_cache_t;

struct assets_shader {
	assets_shader_id_t id;
	char name[SHADER_NAME_MAX_SIZE];
	char vs_filename[SHADER_FILENAME_MAX_SIZE];
	char fs_filename[SHADER_FILENAME_MAX_SIZE];
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

#endif // !ASSET_SHADER_H
