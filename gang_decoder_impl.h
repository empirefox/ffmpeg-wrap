#ifndef GANG_GANG_DECODER_H
#define GANG_GANG_DECODER_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <libavcodec/avcodec.h> 
#include <libavformat/avformat.h> 

#include "stdint.h"

struct gang_decoder {
	char* url;

	//vidio
	int width;
	int height;
	int fps;
	enum AVPixelFormat pix_fmt;
	// audio

	uint32_t sample_rate;
	uint8_t channels;
	int8_t bytesPerSample;
	int is_audio_planar_;

	uint8_t *video_dst_data[4];
	int video_dst_linesize[4];
	int video_dst_bufsize;

	AVFormatContext *i_fmt_ctx;

	AVCodecContext* video_dec_ctx;

	AVCodecContext* audio_dec_ctx;

	unsigned video_stream_index;
	unsigned audio_stream_index;

	AVPacket i_pkt;

	// and more
};

// create gang_decode with given url
struct gang_decoder* new_gang_decoder(const char* url);

// get best format
int init_gang_decoder(struct gang_decoder* decoder);

// prepare AVCodecContext... and store to struct
int start_gang_decode(struct gang_decoder* decoder);

// Read a single frame.
// WebRTC support: I420, YUY2, UYVY, ARGB.
// I420 will not be converted again, other format will not be supported
// until changing the api(init_gang_decoder) to output the format to c++.
// For HD avoid YU12 which is a software conversion and has 2 bugs.
//
// output: *data, need allocate and assign to **data.(do not free it)
//         size
// return: -1->EOF, 0->error, 1->video, 2->audio
int gang_decode_next_frame(
		struct gang_decoder* decoder,
		void **data,
		int *size);

// disconnect from remote stream and free AVCodecContext...
void stop_gang_decode(struct gang_decoder* decoder);

// free gang_decoder
void free_gang_decode(struct gang_decoder* decoder);

#ifdef __cplusplus
} // closing brace for extern "C"
#endif

#endif // GANG_GANG_DECODER_H
