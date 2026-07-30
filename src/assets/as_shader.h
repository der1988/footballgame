#ifndef ASSET_SHADER_H
#define ASSET_SHADER_H

#include "types.h"
#include "memory/memory.h"

#define SHADER_NAME_MAX_SIZE	 50
#define SHADER_FILENAME_MAX_SIZE 50

// Shader Cache
typedef u32 asset_shader_id_t;

typedef struct asset_shader_cache asset_shader_cache_t;

struct asset_shader {
	asset_shader_id_t id;
	char name[SHADER_NAME_MAX_SIZE];
	char vs_filename[SHADER_FILENAME_MAX_SIZE];
	char fs_filename[SHADER_FILENAME_MAX_SIZE];
	shader_t shader;
};

asset_shader_cache_t *asset_shader_cache_create(memory_arena_handle_t *memory_arena_handle);
struct asset_shader *asset_shader_load(asset_shader_cache_t *handle,
									   const char *name,
									   const char *vs_filename,
									   const char *fs_filename);
void asset_shader_cache_destroy(asset_shader_cache_t *handle);
struct asset_shader *asset_shader_get_by_id(asset_shader_cache_t *handle, u32 id);
struct asset_shader *asset_shader_get_by_name(asset_shader_cache_t *handle, const char *name);

#endif // !ASSET_SHADER_H
