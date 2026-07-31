#include "assets/as_sprite_frame.h"
#include <cJSON/cJSON.h>
#include "macro.h"
#include <string.h>
#include "log.h"

size texture_packer_load_from_json(const char *filename,
								  const u32 texture_id,
								  const usize initial_sprite_frame_id,
								  struct asset_sprite_frame *out,
								  usize max_count)
{
	asset_sprite_frame_id_t current_sprite_frame_id = initial_sprite_frame_id;
	size result = -1;
	char *content = NULL;
	cJSON *root = NULL;

	if ((content = LoadFileText(filename)) == NULL) {
		GLOG_ERROR_CTX(LS_ASSETS, filename, "error loading json file");
		goto cleanup;
	}

	if ((root = cJSON_Parse(content)) == NULL) {
		const char *error_ptr = cJSON_GetErrorPtr();
		if (error_ptr) {
			GLOGF_ERROR_CTX(LS_ASSETS, filename, "error parsing json file: %s", error_ptr);
		} else {
			GLOG_ERROR_CTX(LS_ASSETS, filename, "error parsing json file");
		}
		goto cleanup;
	}

	cJSON *frames = cJSON_GetObjectItemCaseSensitive(root, "frames");
	if (!cJSON_IsArray(frames)) {
		GLOG_ERROR_CTX(LS_ASSETS, filename, "error parsing json file: 'frames' is not an array");
		goto cleanup;
	}

	usize count = 0;
	struct asset_sprite_frame *current_sprite_frame = out;

	const cJSON *sprite_frame = NULL;
	cJSON_ArrayForEach(sprite_frame, frames)
	{
		if (count >= max_count) {
			GLOG_ERROR_CTX(LS_ASSETS, filename, "sprite frame count exceeds max_count");
			goto cleanup;
		}

		char *name = JSON_GET_STRING(sprite_frame, "filename");
		b32 rotated = JSON_GET_INT(sprite_frame, "rotated");
		b32 trimmed = JSON_GET_INT(sprite_frame, "trimmed");

		cJSON *frame = cJSON_GetObjectItemCaseSensitive(sprite_frame, "frame");
		if (!frame) {
			GLOG_ERROR_CTX(LS_ASSETS, filename, "missing 'frame' object in sprite frame");
			goto cleanup;
		}
		i32 frame_x = JSON_GET_INT(frame, "x");
		i32 frame_y = JSON_GET_INT(frame, "y");
		i32 frame_w = JSON_GET_INT(frame, "w");
		i32 frame_h = JSON_GET_INT(frame, "h");

		cJSON *sprite_source_size =
			cJSON_GetObjectItemCaseSensitive(sprite_frame, "spriteSourceSize");
		if (!sprite_source_size) {
			GLOG_ERROR_CTX(LS_ASSETS, filename, "missing 'spriteSourceSize' object in sprite frame");
			goto cleanup;
		}
		i32 sprite_source_size_x = JSON_GET_INT(sprite_source_size, "x");
		i32 sprite_source_size_y = JSON_GET_INT(sprite_source_size, "y");
		i32 sprite_source_size_w = JSON_GET_INT(sprite_source_size, "w");
		i32 sprite_source_size_h = JSON_GET_INT(sprite_source_size, "h");

		cJSON *source_size = cJSON_GetObjectItemCaseSensitive(sprite_frame, "sourceSize");
		if (!source_size) {
			GLOG_ERROR_CTX(LS_ASSETS, filename, "missing 'sourceSize' object in sprite frame");
			goto cleanup;
		}
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

		struct asset_sprite_frame *sf = current_sprite_frame++;
		sf->id = current_sprite_frame_id++;
		strncpy(sf->name, name, SPRITE_FRAME_NAME_MAX_SIZE);
		sf->name[SPRITE_FRAME_NAME_MAX_SIZE - 1] = '\0';
		sf->frame = (Rectangle){frame_x, frame_y, frame_w, frame_h};
		sf->trimmed = trimmed;
		sf->rotated = rotated;
		sf->sprite_source_size = (Rectangle){
			sprite_source_size_x, sprite_source_size_y, sprite_source_size_w, sprite_source_size_h};
		sf->source_size = (Vector2){source_size_w, source_size_h};
		sf->pivot = (Vector2){pivot_x, pivot_y};
		sf->texture_id = texture_id;
		count++;
	}

	GLOGF_INFO_CTX(LS_ASSETS, filename, "loaded %zu frames from texture packer json file", count);
	result = (size)count;

cleanup:
	if (root) cJSON_Delete(root);
	if (content) UnloadFileText(content);
	return result;
}
