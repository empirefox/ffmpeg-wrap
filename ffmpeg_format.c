#include "ffmpeg_format.h"
#include "macrologger.h"

/**
 * From transcode_aac.c
 * Convert an error code into a text message.
 * @param error Error code to be converted
 * @return Corresponding error text (not thread-safe)
 */
const char *get_error_text(const int error) {
	static char error_buffer[255];
	av_strerror(error, error_buffer, sizeof(error_buffer));
	return error_buffer;
}

/**
 * From demuxing_decoding.c
 * return error
 */
int open_codec_context(
		int *stream_idx,
		AVFormatContext *(*input_format_context),
		enum AVMediaType type) {

	int ret, stream_index;
	AVStream *st;
	AVCodecContext *dec_ctx = NULL;
	AVCodec *dec = NULL;

	ret = av_find_best_stream((*input_format_context), type, -1, -1, NULL, 0);
	if (ret < 0) {
		LOG_INFO(
				"Could not find %s stream in input file '%s'",
				av_get_media_type_string(type),
				(*input_format_context)->filename);
		return ret;
	} else {
		stream_index = ret;
		st = (*input_format_context)->streams[stream_index];

		/* find decoder for the stream */
		dec_ctx = st->codec;
		dec = avcodec_find_decoder(dec_ctx->codec_id);
		if (!dec) {
			LOG_INFO("Failed to find %s codec", av_get_media_type_string(type));
			return AVERROR(EINVAL);
		}

		if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
			LOG_INFO("Failed to open %s codec", av_get_media_type_string(type));
			return ret;
		}
		*stream_idx = stream_index;
	}

	return 0;
}

/**
 * From transcode_aac.c
 * Open an input file and the required decoder.
 * Need close stream if only get basic stream info but not use it.
 * return error
 */
int open_input_file(
		const char *filename,
		AVFormatContext **input_format_context,
		AVStream **video_stream,
		AVStream **audio_stream) {

	int error, video_stream_idx, audio_stream_idx;

	/** Open the input file to read from it. */
	if ((error = avformat_open_input(input_format_context, filename, NULL, NULL)) < 0) {
		LOG_ERROR("Could not open input file '%s' (error '%s')", filename, get_error_text(error));
		*input_format_context = NULL;
		return error;
	}

	/** Get information on the input file (number of streams etc.). */
	if ((error = avformat_find_stream_info(*input_format_context, NULL)) < 0) {
		LOG_INFO("Could not open find stream info (error '%s')", get_error_text(error));
		avformat_close_input(input_format_context);
		return error;
	}

	// From demuxing_decoding.c
	if (open_codec_context(&video_stream_idx, input_format_context, AVMEDIA_TYPE_VIDEO) >= 0) {
		*video_stream = (*input_format_context)->streams[video_stream_idx];
	}

	if (open_codec_context(&audio_stream_idx, input_format_context, AVMEDIA_TYPE_AUDIO) >= 0) {
		*audio_stream = (*input_format_context)->streams[audio_stream_idx];
	}

	if (!(*video_stream) && !(*audio_stream)) {
		LOG_INFO("Could not find any stream");
		avformat_close_input(input_format_context);
		return AVERROR(AVERROR_STREAM_NOT_FOUND);
	}

	LOG_DEBUG("Input file:%s opened.", filename);

	return 0;
}

/**
 * From transcode_aac.c
 * return error
 * Initialize one audio frame for reading from the input file
 */
int init_frame(AVFrame **frame) {
	if (!(*frame = av_frame_alloc())) {
		LOG_INFO("Could not allocate input frame.");
		return AVERROR(ENOMEM);
	}
	return 0;
}
