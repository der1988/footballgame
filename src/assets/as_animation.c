#include "assets/internal/as_animation_cache.h"
#include "assets/as_animation.h"
#include <cJSON/cJSON.h>
#include "memory/memory.h"
#include "types.h"
#include "macro.h"
#include "log/log.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static u16 const MAX_ANIMATIONS = 100;
static u16 const MAX_FRAMES_PER_ANIMATION = 100;

asset_animation_cache_t *asset_animation_cache_create(memory_arena_handle_t *memory_arena_handle)
{
	asset_animation_cache_t *handle =
		memory_arena_push(memory_arena_handle, sizeof(asset_animation_cache_t));
	assert(handle);

	handle->memory_arena_handle = memory_arena_handle;
	handle->cache =
		memory_arena_push(memory_arena_handle, sizeof(struct asset_animation) * MAX_ANIMATIONS);
	handle->count = 0;
	if (handle->cache == NULL) {
		GLOG_ERROR(LS_ASSETS, "animation cache initialisation failed");
		return false;
	}
	handle->initialised = true;

	GLOGF_INFO(LS_ASSETS, "animation cache initialised for %d animations", MAX_ANIMATIONS);

	return handle;
}

void asset_animation_cache_destroy(asset_animation_cache_t *handle)
{
	assert(handle->initialised);
	handle->cache = NULL;
	handle->count = 0;
	handle->initialised = false;
	GLOG_INFO(LS_ASSETS, "animation cache destroyed");
}

static struct asset_animation *load_from_json(const char *filename,
											  const usize initial_animation_id,
											  usize *count,
											  asset_sprite_frame_cache_t *sprite_frame_cache,
											  memory_arena_handle_t *memory_arena_handle)
{
	asset_animation_id_t current_animation_id = initial_animation_id;
	*count = 0;

	char *content;
	if ((content = LoadFileText(filename)) == NULL) {
		GLOG_ERROR_CTX(LS_ASSETS, filename, "error loading json file");
		return NULL;
	}

	cJSON *root = NULL;
	if ((root = cJSON_Parse(content)) == NULL) {
		const char *error_ptr = cJSON_GetErrorPtr();
		if (error_ptr) {
			GLOGF_ERROR_CTX(LS_ASSETS, filename, "error parsing json file: %s", error_ptr);
		} else {
			GLOG_ERROR_CTX(LS_ASSETS, filename, "error parsing json file");
		}
		UnloadFileText(content);
		return NULL;
	}
	UnloadFileText(content);

	// First pass to count animations
	cJSON *entity = NULL;
	cJSON_ArrayForEach(entity, root)
	{
		const cJSON *animation = NULL;
		cJSON_ArrayForEach(animation, entity) { (*count)++; }
	}
	GLOGF_INFO_CTX(LS_ASSETS, filename, "%zu animations to load", *count);

	struct asset_animation *animations = malloc(sizeof(struct asset_animation) * *count);
	struct asset_animation *current_animation = animations;

	asset_sprite_frame_id_t sprite_frames_buffer[MAX_FRAMES_PER_ANIMATION];
	entity = NULL;
	cJSON_ArrayForEach(entity, root)
	{
		const cJSON *animation = NULL;
		cJSON_ArrayForEach(animation, entity)
		{
			f32 animation_delay = JSON_GET_DOUBLE(animation, "delay");
			char *animation_frames = JSON_GET_STRING(animation, "frames");
			char *animation_prefix = JSON_GET_STRING(animation, "prefix");

			struct asset_animation *assets_animation = current_animation++;
			assets_animation->id = current_animation_id++;
			assets_animation->delay = animation_delay;
			int written = snprintf(assets_animation->name,
								   sizeof(assets_animation->name),
								   "%s_%s",
								   entity->string,
								   animation->string);
			assert(written < (int)sizeof(assets_animation->name) && "animation name too long");

			usize frames_count = 0;
			char *frames = NULL;
			char *frame = NULL;
			char frame_name[ANIMATION_FRAME_MAX_SIZE];
			for (frames = animation_frames;; frames = NULL) {
				frame = strtok(frames, ",");

				if (!frame) {
					break;
				}

				u16 frame_as_number = atoi(frame);
				int written = snprintf(frame_name,
									   sizeof(frame_name),
									   "%s_%s%d",
									   entity->string,
									   animation_prefix,
									   frame_as_number);
				assert(written < (int)sizeof(frame_name) && "frame name too long");

				struct asset_sprite_frame *sprite_frame =
					asset_sprite_frame_get_by_name(sprite_frame_cache, frame_name);
				if (!sprite_frame) {
					GLOGF_WARN_CTX(LS_ASSETS,
								   filename,
								   "animation %s sprite frame not found in cache: %s",
								   assets_animation->name,
								   frame_name);
					continue;
				}
				sprite_frames_buffer[frames_count] = sprite_frame->id;

				GLOGF_TRACE_CTX(LS_ASSETS,
								filename,
								"animation frame %s with id %d",
								frame_name,
								sprite_frame->id);

				frames_count++;
				assert(frames_count < MAX_FRAMES_PER_ANIMATION);
			}
			assets_animation->frames_count = frames_count;
			usize size_to_allocate_for_frames_list = sizeof(asset_sprite_frame_id_t) * frames_count;
			assets_animation->frames =
				memory_arena_push(memory_arena_handle, size_to_allocate_for_frames_list);
			assert(assets_animation->frames);
			memcpy(
				assets_animation->frames, sprite_frames_buffer, size_to_allocate_for_frames_list);

			GLOGF_TRACE_CTX(LS_ASSETS,
							filename,
							"loaded animation %s with %d frames",
							assets_animation->name,
							assets_animation->frames_count);
		}
	}
	cJSON_Delete(root);

	GLOGF_INFO_CTX(LS_ASSETS, filename, "loaded %zu animations from json file", *count);
	return animations;
}

i32 asset_animations_load_from_json(asset_animation_cache_t *handle,
									asset_sprite_frame_cache_t *sprite_frame_cache,
									const char *filename)
{
	assert(handle->initialised);

	usize loaded = 0;
	struct asset_animation *animations = load_from_json(
		filename, handle->count, &loaded, sprite_frame_cache, handle->memory_arena_handle);
	assert(loaded <= (MAX_ANIMATIONS - handle->count));

	if (animations) {
		memcpy(&handle->cache[handle->count], animations, sizeof(struct asset_animation) * loaded);
		free(animations);
		handle->count += loaded;
		GLOGF_INFO_CTX(LS_ASSETS, filename, "loaded %zu animations into cache", loaded);
	} else {
		loaded = -1;
		GLOG_ERROR(LS_ASSETS, "failed loading animations from json");
	}

	return loaded;
}

struct asset_animation *asset_animation_get_by_name(asset_animation_cache_t *handle,
													const char *name)
{
	assert(handle->initialised);

	struct asset_animation *animation = NULL;
	for (usize i = 0; i < handle->count; ++i) {
		animation = &handle->cache[i];
		if (strcmp(animation->name, name) == 0) {
			break;
		}
	}
	return animation;
}

struct asset_animation *asset_animation_get_by_id(asset_animation_cache_t *handle,
												  const asset_animation_id_t id)
{
	assert(handle->initialised);
	return &handle->cache[id];
}

asset_animation_id_t asset_animation_id_from_name(asset_animation_cache_t *handle,
												  const char *name,
												  const char *animation_name,
												  memory_arena_handle_t *memory_arena_handle)
{
	// Scratch buffer
	char *final_animation = memory_arena_push(memory_arena_handle, ANIMATION_NAME_MAX_SIZE);
	assert(final_animation);

	snprintf(final_animation, ANIMATION_NAME_MAX_SIZE, "%s_%s", name, animation_name);
	struct asset_animation *animation = asset_animation_get_by_name(handle, final_animation);
	assert(animation);

	return animation->id;
}
