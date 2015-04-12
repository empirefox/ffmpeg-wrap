//#define NO_GANG
#ifdef NO_GANG
#include "gang_decoder_impl.h"

// create gang_decode with given url
struct gang_decoder* new_gang_decoder(const char* url) {
	struct gang_decoder *decoder_ = (struct gang_decoder*) malloc(
			sizeof(struct gang_decoder));
	decoder_->url = url;
	return decoder_;
}

// get best format and store to struct
int init_gang_decoder(struct gang_decoder* decoder_) {
	decoder_->best_width = 800;
	decoder_->best_height = 600;
	decoder_->best_fps = 24;
	return 1;
}

// prepare AVCodecContext... and store to struct
int start_gang_decode(struct gang_decoder* decoder_) {
	return 1;
}

// read single frame, do not use allocate
struct gang_frame* gang_decode_next_frame(struct gang_decoder* decoder_) {
	return NULL;
}

// disconnect from remote stream and free AVCodecContext...
void stop_gang_decode(struct gang_decoder* decoder_) {
}

// free gang_decoder
void free_gang_decode(struct gang_decoder* decoder_) {
	free(decoder_);
}
#endif
