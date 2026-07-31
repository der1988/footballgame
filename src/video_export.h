#ifndef VIDEO_EXPORT_H
#define VIDEO_EXPORT_H

#include "types.h"

struct game_state;

#define VIDEO_EXPORT_OUTPUT_FPS 60

#ifdef HAS_FFMPEG

typedef struct video_export video_export_t;

b32 video_export_range(usize start_frame,
					   usize end_frame,
					   const char *base_output_path,
					   struct game_state *game_state);

#endif // HAS_FFMPEG

#endif // !VIDEO_EXPORT_H
