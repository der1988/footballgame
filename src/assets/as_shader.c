#include "assets/as_shader.h"
#include "memory/memory.h"
#include "types.h"
#include "log.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct asset_shader_cache {
	b32 initialised;
	memory_arena_handle_t *memory_arena_handle;
	struct asset_shader *cache;
	u32 count;
};

static u32 const MAX_SHADERS = 10;

asset_shader_cache_t *asset_shader_cache_create(memory_arena_handle_t *memory_arena_handle)
{
	asset_shader_cache_t *handle =
		memory_arena_push(memory_arena_handle, sizeof(asset_shader_cache_t));
	assert(handle);

	handle->cache =
		memory_arena_push(memory_arena_handle, sizeof(struct asset_shader) * MAX_SHADERS);
	if (handle->cache == NULL) {
		GLOG_ERROR(LS_ASSETS, "shader cache initialisation failed");
		return false;
	}
	handle->initialised = true;
	GLOG_INFO(LS_ASSETS, "shader cache created");
	return handle;
}

struct asset_shader *asset_shader_load(asset_shader_cache_t *handle,
									   const char *name,
									   const char *vs_filename,
									   const char *fs_filename)
{
	assert(handle->initialised);
	assert(name);

	struct asset_shader *assets_shader = NULL;
	assert(handle->initialised && handle->count < MAX_SHADERS);
	shader_t shader = {0};
	if ((shader = LoadShader(vs_filename, fs_filename)).id == 0) {
		GLOG_ERROR(LS_ASSETS, "error loading shader");
	} else {
		assets_shader = &handle->cache[handle->count];
		assets_shader->id = handle->count;
		assets_shader->shader = shader;
		strncpy(assets_shader->name, name, SHADER_NAME_MAX_SIZE);
		if (vs_filename) {
			strncpy(assets_shader->vs_filename, vs_filename, SHADER_FILENAME_MAX_SIZE);
		}
		if (fs_filename) {
			strncpy(assets_shader->fs_filename, fs_filename, SHADER_FILENAME_MAX_SIZE);
		}
		handle->count++;
		GLOGF_INFO_CTX(LS_ASSETS,
					   name,
					   "shader loaded from [vs = %s, fs = %s] with shader.id = %d: %d",
					   vs_filename,
					   fs_filename,
					   assets_shader->shader.id,
					   assets_shader->id);
	}
	return assets_shader;
}

void asset_shader_cache_destroy(asset_shader_cache_t *handle)
{
	assert(handle->initialised);

	struct asset_shader *assets_shader = NULL;
	for (usize i = 0; i < handle->count; ++i) {
		assets_shader = &handle->cache[i];
		UnloadShader(assets_shader->shader);
	}

	handle->cache = NULL;
	handle->count = 0;
	handle->initialised = false;
	GLOG_INFO(LS_ASSETS, "shader cache destroyed");
}

struct asset_shader *asset_shader_get_by_id(asset_shader_cache_t *handle, u32 id)
{
	assert(handle->initialised);
	struct asset_shader *assets_shader = &handle->cache[id];
	GLOGF_TRACE_CTX(
		LS_ASSETS, assets_shader->name, "retrieved shader by id = %d", assets_shader->id);
	return assets_shader;
}

struct asset_shader *asset_shader_get_by_name(asset_shader_cache_t *handle, const char *name)
{
	assert(handle->initialised);
	struct asset_shader *assets_shader = NULL;
	for (usize i = 0; i < handle->count; ++i) {
		assets_shader = &handle->cache[i];
		if (strcmp(assets_shader->name, name) == 0) {
			break;
		}
	}
	GLOGF_TRACE_CTX(
		LS_ASSETS, assets_shader->name, "retrieved shader with id = %d", assets_shader->id);
	return assets_shader;
}
