#ifndef FFMPEG_FORMAT_H_
#define FFMPEG_FORMAT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>
#include <libswresample/swresample.h>

/**
 * From transcode_aac.c
 * Convert an error code into a text message.
 * @param error Error code to be converted
 * @return Corresponding error text (not thread-safe)
 */
const char *get_error_text(const int error);

/**
 * From demuxing_decoding.c
 * return error
 */
int open_codec_context(
		int *stream_idx,
		AVFormatContext *(*input_format_context),
		enum AVMediaType type);

/**
 * From transcode_aac.c
 * Open an input file and the required decoder.
 */
int open_input_file(
		const char *filename,
		AVFormatContext **input_format_context,
		AVStream **video_stream,
		AVStream **audio_stream);

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
		enum AVSampleFormat *s16_status);

/** Initialize one audio frame for reading from the input file */
int init_frame(AVFrame **frame);

void s16p_2_s16(uint8_t* dst, AVFrame *src_frame, int channels);

#ifdef __cplusplus
} // closing brace for extern "C"
#endif

#endif // FFMPEG_FORMAT_H_
