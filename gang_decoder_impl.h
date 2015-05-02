#ifndef GANG_DECODER_IMPL_H_
#define GANG_DECODER_IMPL_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>

#define GANG_EOF -1
#define GANG_ERROR_DATA 0
#define GANG_VIDEO_DATA 1
#define GANG_AUDIO_DATA 2

struct gang_decoder {
	char *url;
	int no_video;
	int no_audio;

	//vidio
	int width;
	int height;
	int fps;
	enum AVPixelFormat pix_fmt;

	// audio
	int sample_rate;
	int channels;
	int bytes_per_sample;
	enum AVSampleFormat s16_status;

	// video decode buff
	uint8_t *video_dst_data[4];
	int video_dst_linesize[4];
	int video_dst_bufsize;

	/**
	 * audio decode buff
	 * Temporary storage for the converted input samples.
	 */
	uint8_t **audio_dst_data;
	int audio_dst_max_nb_samples;
	int audio_dst_linesize;
	int audio_dst_nb_samples;

	AVFormatContext *i_fmt_ctx;
	SwrContext *swr_ctx;

	AVStream *video_stream;
	AVStream *audio_stream;

	AVPacket i_pkt;
	AVFrame *i_frame;
};

void initialize_gang_decoder_globel();

// create gang_decode with given url
struct gang_decoder* new_gang_decoder(const char* url);

// free gang_decoder
void free_gang_decoder(struct gang_decoder* decoder);

// Init all buffer and data that are needed by decoder.
// return error
int open_gang_decoder(struct gang_decoder* decoder);

// Free all memo that opened by open_gang_decoder.
void close_gang_decoder(struct gang_decoder* decoder);

// Read a single frame.
// TODO but not now.
// Support: I420, YUY2, UYVY, ARGB.
// I420 will not be converted again, other format will not be supported
// until changing the api(open_gang_decoder) to output the format to c++.
// For HD avoid YU12 which is a software conversion and has 2 bugs.
//
// output: *data, need allocate and assign to **data.(do not free it)
//         size
// return: -1->EOF, 0->error, 1->video, 2->audio
int gang_decode_next_frame(
		struct gang_decoder* decoder,
		uint8_t **data,
		int *size);

#ifdef __cplusplus
} // closing brace for extern "C"
#endif

#endif // GANG_DECODER_IMPL_H_
