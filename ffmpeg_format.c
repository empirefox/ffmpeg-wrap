#include "ffmpeg_format.h"

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
		fprintf(
				stderr,
				"Could not find %s stream in input file '%s'\n",
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
			fprintf(
					stderr,
					"Failed to find %s codec\n",
					av_get_media_type_string(type));
			return AVERROR(EINVAL);
		}

		if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
			fprintf(
					stderr,
					"Failed to open %s codec\n",
					av_get_media_type_string(type));
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
	if ((error = avformat_open_input(input_format_context, filename, NULL,
	NULL)) < 0) {
		fprintf(
				stderr,
				"Could not open input file '%s' (error '%s')\n",
				filename,
				get_error_text(error));
		*input_format_context = NULL;
		return error;
	}

	/** Get information on the input file (number of streams etc.). */
	if ((error = avformat_find_stream_info(*input_format_context, NULL)) < 0) {
		fprintf(
				stderr,
				"Could not open find stream info (error '%s')\n",
				get_error_text(error));
		avformat_close_input(input_format_context);
		return error;
	}

	// From demuxing_decoding.c
	if (open_codec_context(
			&video_stream_idx,
			input_format_context,
			AVMEDIA_TYPE_VIDEO) >= 0) {
		*video_stream = (*input_format_context)->streams[video_stream_idx];
	}

	if (open_codec_context(
			&audio_stream_idx,
			input_format_context,
			AVMEDIA_TYPE_AUDIO) >= 0) {
		*audio_stream = (*input_format_context)->streams[audio_stream_idx];
	}

	if (!(*video_stream) && !(*audio_stream)) {
		fprintf(
		stderr, "Could not find any stream\n");
		avformat_close_input(input_format_context);
		return AVERROR(AVERROR_STREAM_NOT_FOUND);
	}

	return 0;
}

/**
 * From transcode_aac.c#init_resampler
 * return error
 * If resample_context==NULL, then no need for resample.
 *
 * Initialize the audio resampler based on the input and output codec settings.
 * If the input and output sample formats differ, a conversion is required
 * libswresample takes care of this, but requires initialization.
 */
int init_audio_resampler(
		AVCodecContext *input_codec_context,
		SwrContext **resample_context,
		int *o_chs,
		int *o_sample_rate,
		enum AVSampleFormat *s16_status) {
	int error;

	/**
	 * Create a resampler context for the conversion.
	 * Set the conversion parameters.
	 * Default channel layouts based on the number of channels
	 * are assumed for simplicity (they are sometimes not detected
	 * properly by the demuxer and/or decoder).
	 */
	int i_chs = input_codec_context->channels;
	enum AVSampleFormat i_sample_fmt = input_codec_context->sample_fmt;
	int i_sample_rate = input_codec_context->sample_rate;
	if (i_chs <= 2 && i_sample_rate <= 96000) {
		if (i_sample_fmt == AV_SAMPLE_FMT_S16) {
			*s16_status = AV_SAMPLE_FMT_S16;
			// return no error
			return 0;
		} else if (i_sample_fmt == AV_SAMPLE_FMT_S16P) {
			*s16_status = AV_SAMPLE_FMT_S16P;
			// return no error
			return 0;
		}
	}
	*s16_status = AV_SAMPLE_FMT_NONE;
	*o_chs = i_chs > 2 ? 2 : i_chs;
	*o_sample_rate = i_sample_rate > 96000 ? 44100 : i_sample_rate;

	*resample_context = swr_alloc_set_opts(NULL,
	// output
			av_get_default_channel_layout(*o_chs),
			AV_SAMPLE_FMT_S16,
			*o_sample_rate,

			// input
			av_get_default_channel_layout(i_chs),
			i_sample_fmt,
			i_sample_rate,

			// log
			0,
			NULL);
	if (!(*resample_context)) {
		fprintf(stderr, "Could not allocate resample context\n");
		return AVERROR(ENOMEM);
	}

	/** Open the resampler with the specified parameters. */
	if ((error = swr_init(*resample_context)) < 0) {
		fprintf(stderr, "Could not open resample context\n");
		swr_free(resample_context);
		return error;
	}
	return 0;
}

/**
 * From transcode_aac.c
 * return error
 * Initialize one audio frame for reading from the input file
 */
int init_input_frame(AVFrame **frame) {
	if (!(*frame = av_frame_alloc())) {
		fprintf(stderr, "Could not allocate input frame\n");
		return AVERROR(ENOMEM);
	}
	return 0;
}

void s16p_2_s16(uint8_t* dst, AVFrame *src_frame, int channels) {
	int i, ch, bytesPerSample = 2;
	for (i = 0; i < src_frame->nb_samples; i++) {
		for (ch = 0; ch < channels; ++ch) {
			memcpy(
					dst,
					src_frame->data[ch] + bytesPerSample * i,
					bytesPerSample);
			dst += bytesPerSample;
		}
	}
}
