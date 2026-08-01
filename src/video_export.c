#ifdef HAS_FFMPEG

#include "types.h"
#include "video_export.h"
#include <stdio.h>
#include <sys/stat.h>
#include "game.h"
#include "log.h"
#include "memory.h"
#include "time_machine.h"
#include <assert.h>
#include <raylib.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>

static video_export_t *begin_export(
	memory_arena_handle_t *memory_arena, const char *output_path, u32 width, u32 height, u32 fps);
static void end_export(video_export_t *handle);
static void build_path(const char *base_name, char *out, usize out_size);
static b32 encode_and_write(video_export_t *handle, AVFrame *frame);
static b32 write_frame(video_export_t *handle, const u8 *rgba_data);

struct video_export {
	AVFormatContext *fmt_ctx;
	AVCodecContext *codec_ctx;
	AVStream *stream;
	AVFrame *frame;
	AVPacket *pkt;
	struct SwsContext *sws_ctx;
	u32 width;
	u32 height;
	u64 frame_index;
};

b32 video_export_range(usize start_frame,
					   usize end_frame,
					   const char *base_output_path,
					   struct game_state *game_state)
{
	assert(game_state);
	assert(start_frame <= end_frame);
	assert(base_output_path);

	char output_path[128];
	build_path(base_output_path, output_path, sizeof(output_path));

	struct time_machine *tm = game_state->time_machine;
	u32 export_w = game_state->config.virtual_screen_width * 2;
	u32 export_h = game_state->config.virtual_screen_height * 2;

	render_texture_t export_rt = LoadRenderTexture((int)export_w, (int)export_h);

	u32 output_fps = VIDEO_EXPORT_OUTPUT_FPS;
	// Allocated on MAL_UPDATE — safe because VCR export only restores/renders,
	// never calls update() which would reset this arena.
	video_export_t *handle = begin_export(
		game_state->memory_by_lifetime[MAL_UPDATE], output_path, export_w, export_h, output_fps);
	if (!handle) {
		UnloadRenderTexture(export_rt);
		return false;
	}

	// Save current position to restore after export
	usize saved_pos = tm->current_pos;

	u32 step = TIME_CAPSULES_PER_SECOND / output_fps;
	usize total_capsules = end_frame - start_frame + 1;
	usize total_output_frames = (total_capsules + step - 1) / step;

	// Navigate to start_frame using time_machine_travel (same mechanism as VCR slider)
	size travel_to_start = (size)start_frame - (size)time_machine_current_pos(tm);
	time_machine_travel(tm, travel_to_start, game_state);

	GLOGF_INFO(LS_VIDEO_EXPORT,
			   "exporting %zu frames (capsules %zu-%zu, step %u)",
			   total_output_frames,
			   start_frame,
			   end_frame,
			   step);

	b32 export_successful = true;

	for (usize i = 0; i < total_output_frames; i++) {
		if (i > 0) {
			time_machine_travel(tm, (size)step, game_state);
		}

		// Reset draw arena for each frame
		memory_arena_reset(game_state->memory_by_lifetime[MAL_DRAW]);

		// Build a camera for the 2x export resolution
		camera2d_t export_cam = game_state->virtual_camera;
		export_cam.offset.x *= 2.0f;
		export_cam.offset.y *= 2.0f;
		export_cam.zoom = 2.0f;

		// Update camera target from restored capsule state
		struct component_transform *camera_transform = &game_state->vars.camera_entity->transform;
		export_cam.target.x = (i64)camera_transform->position.x;
		export_cam.target.y = (i64)camera_transform->position.y;

		game_play_draw_scene_to_texture(game_state, export_rt, export_cam, false);

		// Read pixels from GPU
		Image img = LoadImageFromTexture(export_rt.texture);
		ImageFlipVertical(&img);

		if (!write_frame(handle, (u8 *)img.data)) {
			UnloadImage(img);
			GLOGF_ERROR(LS_VIDEO_EXPORT, "encode failed at frame %zu, aborting", i);
			export_successful = false;
			break;
		}
		UnloadImage(img);
	}

	end_export(handle);
	UnloadRenderTexture(export_rt);

	// Restore original position
	time_machine_restore_capsule(tm, saved_pos, game_state);

	if (export_successful)
		GLOGF_INFO(LS_VIDEO_EXPORT, "exported %zu frames to %s", total_output_frames, output_path);

	return export_successful;
}

static video_export_t *begin_export(
	memory_arena_handle_t *memory_arena, const char *output_path, u32 width, u32 height, u32 fps)
{
	video_export_t *handle = memory_arena_push(memory_arena, sizeof(video_export_t));
	if (!handle)
		return NULL;

	handle->width = width;
	handle->height = height;

	// Output format context
	int ret = avformat_alloc_output_context2(&handle->fmt_ctx, NULL, "mp4", output_path);
	if (ret < 0 || !handle->fmt_ctx) {
		GLOG_ERROR(LS_VIDEO_EXPORT, "could not create output context");
		goto fail;
	}

	// Find H.264 encoder
	const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!codec) {
		GLOG_ERROR(LS_VIDEO_EXPORT, "H.264 encoder not found");
		goto fail;
	}

	// Create stream
	handle->stream = avformat_new_stream(handle->fmt_ctx, NULL);
	if (!handle->stream) {
		GLOG_ERROR(LS_VIDEO_EXPORT, "could not create stream");
		goto fail;
	}

	// Codec context
	handle->codec_ctx = avcodec_alloc_context3(codec);
	if (!handle->codec_ctx) {
		GLOG_ERROR(LS_VIDEO_EXPORT, "could not allocate codec context");
		goto fail;
	}

	handle->codec_ctx->width = (int)width;
	handle->codec_ctx->height = (int)height;
	handle->codec_ctx->time_base = (AVRational){1, (int)fps};
	handle->codec_ctx->framerate = (AVRational){(int)fps, 1};
	handle->codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
	handle->codec_ctx->gop_size = 12;
	handle->codec_ctx->bit_rate = 4000000;

	av_opt_set(handle->codec_ctx->priv_data, "preset", "medium", 0);
	av_opt_set(handle->codec_ctx->priv_data, "crf", "20", 0);

	if (handle->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		handle->codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	ret = avcodec_open2(handle->codec_ctx, codec, NULL);
	if (ret < 0) {
		GLOG_ERROR(LS_VIDEO_EXPORT, "could not open codec");
		goto fail;
	}

	ret = avcodec_parameters_from_context(handle->stream->codecpar, handle->codec_ctx);
	if (ret < 0) {
		GLOG_ERROR(LS_VIDEO_EXPORT, "could not copy codec parameters");
		goto fail;
	}

	handle->stream->time_base = handle->codec_ctx->time_base;

	// Open output file
	if (!(handle->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
		ret = avio_open(&handle->fmt_ctx->pb, output_path, AVIO_FLAG_WRITE);
		if (ret < 0) {
			GLOGF_ERROR(LS_VIDEO_EXPORT, "could not open output file: %s", output_path);
			goto fail;
		}
	}

	ret = avformat_write_header(handle->fmt_ctx, NULL);
	if (ret < 0) {
		GLOG_ERROR(LS_VIDEO_EXPORT, "could not write header");
		goto fail;
	}

	// Allocate frame (YUV420P)
	handle->frame = av_frame_alloc();
	if (!handle->frame)
		goto fail;

	handle->frame->format = AV_PIX_FMT_YUV420P;
	handle->frame->width = (int)width;
	handle->frame->height = (int)height;

	ret = av_frame_get_buffer(handle->frame, 0);
	if (ret < 0) {
		GLOG_ERROR(LS_VIDEO_EXPORT, "could not allocate frame buffer");
		goto fail;
	}

	// Allocate packet
	handle->pkt = av_packet_alloc();
	if (!handle->pkt)
		goto fail;

	// RGBA -> YUV420P converter
	handle->sws_ctx = sws_getContext((int)width,
									 (int)height,
									 AV_PIX_FMT_RGBA,
									 (int)width,
									 (int)height,
									 AV_PIX_FMT_YUV420P,
									 SWS_BILINEAR,
									 NULL,
									 NULL,
									 NULL);
	if (!handle->sws_ctx) {
		GLOG_ERROR(LS_VIDEO_EXPORT, "could not create sws context");
		goto fail;
	}

	handle->frame_index = 0;

	GLOGF_INFO(
		LS_VIDEO_EXPORT, "started export to %s (%ux%u @ %u fps)", output_path, width, height, fps);

	return handle;

fail:
	end_export(handle);
	return NULL;
}

static void end_export(video_export_t *handle)
{
	if (!handle)
		return;

	// Flush encoder
	if (handle->codec_ctx)
		encode_and_write(handle, NULL);

	// Write trailer
	if (handle->fmt_ctx && handle->fmt_ctx->pb)
		av_write_trailer(handle->fmt_ctx);

	// Cleanup
	if (handle->sws_ctx)
		sws_freeContext(handle->sws_ctx);
	if (handle->pkt)
		av_packet_free(&handle->pkt);
	if (handle->frame)
		av_frame_free(&handle->frame);
	if (handle->codec_ctx)
		avcodec_free_context(&handle->codec_ctx);
	if (handle->fmt_ctx) {
		if (!(handle->fmt_ctx->oformat->flags & AVFMT_NOFILE) && handle->fmt_ctx->pb)
			avio_closep(&handle->fmt_ctx->pb);
		avformat_free_context(handle->fmt_ctx);
	}

	GLOG_INFO(LS_VIDEO_EXPORT, "export finished");
}

static b32 encode_and_write(video_export_t *handle, AVFrame *frame)
{
	int ret = avcodec_send_frame(handle->codec_ctx, frame);
	if (ret < 0) {
		GLOG_ERROR(LS_VIDEO_EXPORT, "error sending frame to encoder");
		return false;
	}

	while (ret >= 0) {
		ret = avcodec_receive_packet(handle->codec_ctx, handle->pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			break;
		if (ret < 0) {
			GLOG_ERROR(LS_VIDEO_EXPORT, "error receiving packet from encoder");
			return false;
		}

		av_packet_rescale_ts(handle->pkt, handle->codec_ctx->time_base, handle->stream->time_base);
		handle->pkt->stream_index = handle->stream->index;

		ret = av_interleaved_write_frame(handle->fmt_ctx, handle->pkt);
		if (ret < 0) {
			GLOG_ERROR(LS_VIDEO_EXPORT, "error writing packet to output");
			return false;
		}
	}

	return true;
}

static b32 write_frame(video_export_t *handle, const u8 *rgba_data)
{
	assert(handle);

	int ret = av_frame_make_writable(handle->frame);
	if (ret < 0)
		return false;

	const u8 *src_data[1] = {rgba_data};
	int src_linesize[1] = {(int)(handle->width * 4)};

	sws_scale(handle->sws_ctx,
			  src_data,
			  src_linesize,
			  0,
			  (int)handle->height,
			  handle->frame->data,
			  handle->frame->linesize);

	handle->frame->pts = (i64)handle->frame_index++;

	return encode_and_write(handle, handle->frame);
}

// Formats base_name to base_name_000.mp4
static void build_path(const char *base_name, char *out, usize out_size)
{
	struct stat st;
	for (u32 seq = 0; seq < 1000; seq++) {
		snprintf(out, out_size, "%s_%03u.mp4", base_name, seq);
		if (stat(out, &st) != 0) {
			return;
		}
	}
	// Fallback if all 1000 taken
	snprintf(out, out_size, "%s_999.mp4", base_name);
}

#endif // HAS_FFMPEG
