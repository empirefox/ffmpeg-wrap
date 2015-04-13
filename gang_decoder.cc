#include  "gang_decoder.h"
#include  "gang_decoder_impl.h"

namespace gang {

GangDecoder::GangDecoder(const char* url,
		VideoFrameObserver* video_frame_observer,
		AudioFrameObserver* audio_frame_observer) :
		decoder_(::new_gang_decoder(url)), video_frame_observer_(
				video_frame_observer), audio_frame_observer_(
				audio_frame_observer) {
}

GangDecoder::~GangDecoder() {
	::free_gang_decode(decoder_);
}

bool GangDecoder::Init() {
	bool ok = false;
	int* best_width;
	int* best_height;
	int* best_fps;
	if (::init_gang_decoder(decoder_, best_width, best_height, best_fps)) {
		video_frame_observer_->OnBestFormat(*best_width, *best_height,
				*best_fps);
		ok = true;
	}
	return ok;
}

bool GangDecoder::Start() {
	return ::start_gang_decode(decoder_);
}

void GangDecoder::Stop() {
	::stop_gang_decode(decoder_);
}

void GangDecoder::NextFrameLoop() {
	uint8 **data;
	int *size;
	switch (::gang_decode_next_frame(decoder_, data, size)) {
	case 1:
		video_frame_observer_->OnVideoFrame(*data, static_cast<uint32>(*size));
		break;
	case 2:
		audio_frame_observer_->OnAudioFrame(*data, static_cast<uint32>(*size));
		break;
	default:
		break;
	}
}

}  // namespace gang
