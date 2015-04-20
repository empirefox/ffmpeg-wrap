#include  "gang_decoder.h"
#include  "gang_decoder_impl.h"
#include <iostream>

namespace gang {

GangDecoder::GangDecoder(const std::string& url,
		VideoFrameObserver* video_frame_observer,
		AudioFrameObserver* audio_frame_observer) :
		decoder_(::new_gang_decoder(url.c_str())), video_frame_observer_(
				video_frame_observer), audio_frame_observer_(
				audio_frame_observer), connected_(false), best_width_(640), best_height_(
				480), best_fps_(30) {
}

GangDecoder::~GangDecoder() {
	Stop();
	::free_gang_decode(decoder_);
}

void GangDecoder::Stop(){
	disconnect();
	rtc::Thread::Stop();
}

bool GangDecoder::Init() {
	bool ok = false;
	if (::init_gang_decoder(decoder_, &best_width_, &best_height_, &best_fps_)
			== 1) {
		ok = true;
	}
	return ok;
}

void GangDecoder::GetBestFormat(int* width, int* height, int* fps) {
	*width = best_width_;
	*height = best_height_;
	*fps = best_fps_;
}

void GangDecoder::Run() {
	if (connect()) {
		printf("failed to run\n");
		return;
	}
	printf("start to run\n");
	while (connected_ && NextFrameLoop()) {
	}
	disconnect();
}

bool GangDecoder::connect() {
	rtc::CritScope cs(&crit_);
	if (!connected_) {
		printf("::start_gang_decode start\n");
		connected_ = ::start_gang_decode(decoder_) == 0;
		printf("::start_gang_decode end\n");
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
	uint8 *data = 0;
	int size = 0;
	switch (::gang_decode_next_frame(decoder_, &data, &size)) {
	case 1:
		std::cout << "data size: " << sizeof(data) / sizeof(uint8_t) << std::endl;
		if (video_frame_observer_)
			video_frame_observer_->OnVideoFrame(reinterpret_cast<uint8*>(data),
					static_cast<uint32>(size));
		break;
	case 2:
		if (audio_frame_observer_)
			audio_frame_observer_->OnAudioFrame(reinterpret_cast<uint8*>(data),
					static_cast<uint32>(size));
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
