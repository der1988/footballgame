#ifndef TEXTURE_PACKER_LOADER_H
#define TEXTURE_PACKER_LOADER_H

#include "types.h"

struct asset_sprite_frame *texture_packer_load_from_json(const char *filename,
														 const u32 texture_id,
														 const usize initial_sprite_frame_id,
														 usize *count);

#endif // !TEXTURE_PACKER_LOADER_H
