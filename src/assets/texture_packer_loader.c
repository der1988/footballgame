#include "asset_sprite_frame.h"
#include "libs/cJSON/cJSON.h"
#include "macro.h"
#include <stdlib.h>
#include <string.h>
#include "log.h"

struct assets_sprite_frame *texture_packer_load_from_json(const char *filename,
														  const u32 texture_id,
														  const usize initial_sprite_frame_id,
														  usize *count)
{
	assets_sprite_frame_id_t current_sprite_frame_id = initial_sprite_frame_id;
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

	cJSON *frames = cJSON_GetObjectItemCaseSensitive(root, "frames");
	if (!cJSON_IsArray(frames)) {
		const char *error_ptr = cJSON_GetErrorPtr();
		if (error_ptr) {
			GLOGF_ERROR_CTX(LS_ASSETS, filename, "error parsing json file: %s", error_ptr);
		} else {
			GLOG_ERROR_CTX(LS_ASSETS, filename, "error parsing json file");
		}
		return NULL;
	}

	*count = cJSON_GetArraySize(frames);
	struct assets_sprite_frame *sprite_frames = malloc(sizeof(struct assets_sprite_frame) * *count);
	struct assets_sprite_frame *current_sprite_frame = sprite_frames;

	const cJSON *sprite_frame = NULL;
	cJSON_ArrayForEach(sprite_frame, frames)
	{
		char *name = JSON_GET_STRING(sprite_frame, "filename");
		b32 rotated = JSON_GET_INT(sprite_frame, "rotated");
		b32 trimmed = JSON_GET_INT(sprite_frame, "trimmed");

		cJSON *frame = cJSON_GetObjectItemCaseSensitive(sprite_frame, "frame");
		i32 frame_x = JSON_GET_INT(frame, "x");
		i32 frame_y = JSON_GET_INT(frame, "y");
		i32 frame_w = JSON_GET_INT(frame, "w");
		i32 frame_h = JSON_GET_INT(frame, "h");

		cJSON *sprite_source_size =
			cJSON_GetObjectItemCaseSensitive(sprite_frame, "spriteSourceSize");
		i32 sprite_source_size_x = JSON_GET_INT(sprite_source_size, "x");
		i32 sprite_source_size_y = JSON_GET_INT(sprite_source_size, "y");
		i32 sprite_source_size_w = JSON_GET_INT(sprite_source_size, "w");
		i32 sprite_source_size_h = JSON_GET_INT(sprite_source_size, "h");

		cJSON *source_size = cJSON_GetObjectItemCaseSensitive(sprite_frame, "sourceSize");
		i32 source_size_w = JSON_GET_INT(source_size, "w");
		i32 source_size_h = JSON_GET_INT(source_size, "h");

		// Pivot is optional
		f64 pivot_x = 0;
		f64 pivot_y = 0;
		cJSON *pivot = cJSON_GetObjectItemCaseSensitive(sprite_frame, "pivot");
		if (pivot) {
			pivot_x = JSON_GET_DOUBLE(pivot, "x");
			pivot_y = JSON_GET_DOUBLE(pivot, "y");
		}

		struct assets_sprite_frame *sprite_frame = current_sprite_frame++;
		sprite_frame->id = current_sprite_frame_id++;
		strncpy(sprite_frame->name, name, SPRITE_FRAME_NAME_MAX_SIZE);
		sprite_frame->frame = (Rectangle){frame_x, frame_y, frame_w, frame_h};
		sprite_frame->trimmed = trimmed;
		sprite_frame->rotated = rotated;
		sprite_frame->sprite_source_size = (Rectangle){
			sprite_source_size_x, sprite_source_size_y, sprite_source_size_w, sprite_source_size_h};
		sprite_frame->source_size = (Vector2){source_size_w, source_size_h};
		sprite_frame->pivot = (Vector2){pivot_x, pivot_y};
		sprite_frame->texture_id = texture_id;
	}
	cJSON_Delete(root);

	GLOGF_INFO_CTX(LS_ASSETS, filename, "loaded %zu frames from texture packer json file", *count);

	return sprite_frames;
}
