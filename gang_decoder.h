#ifndef GANG_GANG_DECODER_H
#define GANG_GANG_DECODER_H
#ifdef __cplusplus
extern "C" {
#endif

struct gang_decoder {
	char* url;
	int best_width;
	int best_height;
	int best_fps;

	// and more
};

struct gang_frame {
	uint8_t* data;
	int size;
	int is_video;
};

// create gang_decode with given url
struct gang_decoder* new_gang_decoder(const char* url);

// get best format and store to struct
int init_gang_decoder(struct gang_decoder* decoder_);

// prepare AVCodecContext... and store to struct
int start_gang_decode(struct gang_decoder* decoder_);

// read single frame, do not use allocate
struct gang_frame gang_decode_next_frame(struct gang_decoder* decoder_);

// disconnect from remote stream and free AVCodecContext...
void stop_gang_decode(struct gang_decoder* decoder_);

// free gang_decoder
void free_gang_decode(struct gang_decoder* decoder_);

#ifdef __cplusplus
} // closing brace for extern "C"
#endif

#endif // GANG_GANG_DECODER_H
