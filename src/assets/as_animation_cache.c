#include "assets_internal.h"
#include "assets.h"
#include "libs/cJSON/cJSON.h"
#include "memory.h"
#include "types.h"
#include "utils.h"
#include "log.h"
#include <assert.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED_PARAM __attribute__((unused))

static u16 const MAX_ANIMATIONS = 100;
static u16 const MAX_FRAMES_PER_ANIMATION = 100;

AssetsAnimationCache *assets_animation_cache_create(memory_arena_handle_t *memory_arena_handle)
{
    AssetsAnimationCache *handle =
	memory_arena_push(memory_arena_handle, sizeof(AssetsAnimationCache));
    assert(handle);

    handle->memory_arena_handle = memory_arena_handle;
    handle->cache =
	memory_arena_push(memory_arena_handle, sizeof(struct AssetsAnimation) * MAX_ANIMATIONS);
    handle->count = 0;
    if (handle->cache == NULL)
    {
	GLOG_ERROR(LS_ASSETS, "animation cache initialisation failed");
	return false;
    }
    handle->initialised = true;

    GLOGF_INFO(LS_ASSETS, "animation cache initialised for %d animations", MAX_ANIMATIONS);

    return handle;
}

void assets_animation_cache_destroy(AssetsAnimationCache *handle)
{
    assert(handle->initialised);
    handle->cache = NULL;
    handle->count = 0;
    handle->initialised = false;
    GLOG_INFO(LS_ASSETS, "animation cache destroyed");
}

static struct AssetsAnimation *load_from_json(const char *filename,
					      const usize initial_animation_id, usize *count,
					      AssetsSpriteFrameCache *sprite_frame_cache,
					      memory_arena_handle_t *memory_arena_handle)
{
    AssetsAnimationId current_animation_id = initial_animation_id;
    *count = 0;

    char *content;
    if ((content = LoadFileText(filename)) == NULL)
    {
	GLOG_ERROR_CTX(LS_ASSETS, filename, "error loading json file");
	return NULL;
    }

    cJSON *root = NULL;
    if ((root = cJSON_Parse(content)) == NULL)
    {
	const char *error_ptr = cJSON_GetErrorPtr();
	if (error_ptr)
	{
	    GLOGF_ERROR_CTX(LS_ASSETS, filename, "error parsing json file: %s", error_ptr);
	}
	else
	{
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

    struct AssetsAnimation *animations = malloc(sizeof(struct AssetsAnimation) * *count);
    struct AssetsAnimation *current_animation = animations;

    AssetsSpriteFrameId sprite_frames_buffer[MAX_FRAMES_PER_ANIMATION];
    entity = NULL;
    cJSON_ArrayForEach(entity, root)
    {
	const cJSON *animation = NULL;
	cJSON_ArrayForEach(animation, entity)
	{
	    f32 animation_delay = JSON_GET_DOUBLE(animation, "delay");
	    char *animation_frames = JSON_GET_STRING(animation, "frames");
	    char *animation_prefix = JSON_GET_STRING(animation, "prefix");

	    struct AssetsAnimation *assets_animation = current_animation++;
	    assets_animation->id = current_animation_id++;
	    assets_animation->delay = animation_delay;
	    strncpy(assets_animation->name, entity->string, sizeof(assets_animation->name) - 1);
	    strcat(assets_animation->name + strlen(assets_animation->name), "_");
	    strcat(assets_animation->name + strlen(assets_animation->name), animation->string);

	    usize frames_count = 0;
	    char *frames = NULL;
	    char *frame = NULL;
	    char frame_name_format[MaxStringSize];
	    char frame_name[MaxStringSize];
	    for (frames = animation_frames;; frames = NULL)
	    {
		frame = strtok(frames, ",");

		if (!frame)
		{
		    break;
		}

		u16 frame_as_number = atoi(frame);
		strncpy(frame_name_format, entity->string, sizeof(frame_name) - 1);
		strcat(frame_name_format + strlen(frame_name_format), "_");
		strcat(frame_name_format + strlen(frame_name_format), animation_prefix);
		strcat(frame_name_format + strlen(frame_name_format), "%d");
		snprintf(frame_name, sizeof(frame_name) - 1, frame_name_format, frame_as_number);

		struct AssetsSpriteFrame *sprite_frame =
		    assets_sprite_frame_get_by_name(sprite_frame_cache, frame_name);
		if (!sprite_frame)
		{
		    GLOGF_WARN_CTX(LS_ASSETS, filename,
				   "animation %s sprite frame not found in cache: %s",
				   assets_animation->name, frame_name);
		    continue;
		}
		sprite_frames_buffer[frames_count] = sprite_frame->id;

		GLOGF_TRACE_CTX(LS_ASSETS, filename, "animation frame %s with id %d", frame_name,
				sprite_frame->id);

		frames_count++;
		assert(frames_count < MAX_FRAMES_PER_ANIMATION);
	    }
	    assets_animation->frames_count = frames_count;
	    usize size_to_allocate_for_frames_list = sizeof(AssetsSpriteFrameId) * frames_count;
	    assets_animation->frames =
		memory_arena_push(memory_arena_handle, size_to_allocate_for_frames_list);
	    memcpy(assets_animation->frames, sprite_frames_buffer,
		   size_to_allocate_for_frames_list);

	    GLOGF_TRACE_CTX(LS_ASSETS, filename, "loaded animation %s with %d frames",
			    assets_animation->name, assets_animation->frames_count);
	}
    }
    cJSON_Delete(root);

    GLOGF_INFO_CTX(LS_ASSETS, filename, "loaded %zu animations from json file", *count);
    return animations;
}

i32 assets_animations_load_from_json(AssetsAnimationCache *handle,
				     AssetsSpriteFrameCache *sprite_frame_cache,
				     const char *filename)
{
    assert(handle->initialised);

    usize loaded = 0;
    struct AssetsAnimation *animations = load_from_json(
	filename, handle->count, &loaded, sprite_frame_cache, handle->memory_arena_handle);
    if (loaded > (MAX_ANIMATIONS - handle->count))
    {
	GLOG_ERROR(LS_ASSETS, "animations cache is too small");
    }
    else if (animations)
    {
	memcpy(&handle->cache[handle->count], animations, sizeof(struct AssetsAnimation) * loaded);
	free(animations);
	handle->count += loaded;
	GLOGF_INFO_CTX(LS_ASSETS, filename, "loaded %zu animations into cache", loaded);
	return loaded;
    }
    else
    {
	GLOG_ERROR(LS_ASSETS, "failed loading animations from json");
    }

    return -1;
}

struct AssetsAnimation *assets_animation_get_by_name(AssetsAnimationCache *handle, const char *name)
{
    assert(handle->initialised);

    struct AssetsAnimation *animation = NULL;
    for (usize i = 0; i < handle->count; ++i)
    {
	animation = &handle->cache[i];
	if (strcmp(animation->name, name) == 0)
	{
	    break;
	}
    }
    return animation;
}

struct AssetsAnimation *assets_animation_get_by_id(AssetsAnimationCache *handle,
						   const AssetsAnimationId id)
{
    assert(handle->initialised);
    return &handle->cache[id];
}
