#include  "gang_decoder.h"
#include  "gang_decoder_impl.h"

namespace gang {

GangDecoder::GangDecoder(const char* url,
		VideoFrameObserver* video_frame_observer,
		AudioFrameObserver* audio_frame_observer) :
		decoder_(::new_gang_decoder(url)), video_frame_observer_(
				video_frame_observer), audio_frame_observer_(
				audio_frame_observer), connected_(false) {
}

GangDecoder::~GangDecoder() {
	disconnect();
	Stop();
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

void GangDecoder::Run() {
	if (connect()) {
		return;
	}
	while (connected_ && NextFrameLoop()) {
	}
	disconnect();
}

bool GangDecoder::connect() {
	rtc::CritScope cs(&crit_);
	if (!connected_) {
		connected_ = ::start_gang_decode(decoder_) == 0;
	}
	return connected_;
}

// Check if Run() is finished.
bool GangDecoder::Connected() {
	rtc::CritScope cs(&crit_);
	return connected_;
}

void GangDecoder::disconnect() {
	rtc::CritScope cs(&crit_);
	if (connected_) {
		::stop_gang_decode(decoder_);
		connected_ = false;
	}
}

bool GangDecoder::NextFrameLoop() {
	bool is_eof = false;
	uint8 **data;
	int *size;
	switch (::gang_decode_next_frame(decoder_, data, size)) {
	case 1:
		if (video_frame_observer_)
			video_frame_observer_->OnVideoFrame(reinterpret_cast<uint8*>(*data),
					static_cast<uint32>(*size));
		break;
	case 2:
		if (audio_frame_observer_)
			audio_frame_observer_->OnAudioFrame(reinterpret_cast<uint8*>(*data),
					static_cast<uint32>(*size));
		break;
	case -1:
		is_eof = true;
		break;
	default:
		break;
	}
	return is_eof;
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
