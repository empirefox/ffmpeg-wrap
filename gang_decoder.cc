#include  "gang_decoder.h"
#include  "gang_decoder_impl.h"

namespace gang {

enum {
	MSG_START_DECODE, MSG_STOP_DECODE,
};

GangDecoder::GangDecoder(const char* url,
		VideoFrameObserver* video_frame_observer,
		AudioFrameObserver* audio_frame_observer) :
		decoder_(::new_gang_decoder(url)), video_frame_observer_(
				video_frame_observer), audio_frame_observer_(
				audio_frame_observer), running_(false) {
}

GangDecoder::~GangDecoder() {
	::free_gang_decode(decoder_);
}

bool GangDecoder::Init() {
	bool ok = false;
	int* best_width;
	int* best_height;
	int* best_fps;
	if (::init_gang_decoder(decoder_, best_width, best_height, best_fps) == 1) {
		video_frame_observer_->OnBestFormat(*best_width, *best_height,
				*best_fps);
		ok = true;
	}
	return ok;
}

bool GangDecoder::Start() {
	return ::start_gang_decode(decoder_) == 1;
}

void GangDecoder::Stop() {
	::stop_gang_decode(decoder_);
}

void GangDecoder::NextFrameLoop() {
	uint8 **data;
	int *size;
	switch (::gang_decode_next_frame(decoder_, data, size)) {
	case 1:
		if (video_frame_observer_ != NULL)
			video_frame_observer_->OnVideoFrame(reinterpret_cast<uint8*>(*data),
					static_cast<uint32>(*size));
		break;
	case 2:
		if (audio_frame_observer_ != NULL)
			audio_frame_observer_->OnAudioFrame(reinterpret_cast<uint8*>(*data),
					static_cast<uint32>(*size));
		break;
	default:
		break;
	}
}

void GangDecoder::OnMessage(rtc::Message* msg) {
	switch (msg->message_id) {
	case MSG_START_DECODE:
		// TODO
		break;
	default:
		break;
	}
}

void GangDecoder::SetVideoFrameObserver(
		VideoFrameObserver* video_frame_observer) {
	video_frame_observer_ = video_frame_observer;
}

void GangDecoder::SetAudioFrameObserver(
		AudioFrameObserver* audio_frame_observer) {
	audio_frame_observer_ = audio_frame_observer;
}

}  // namespace gang
