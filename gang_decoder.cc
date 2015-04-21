#include  "gang_decoder.h"
#include  "gang_decoder_impl.h"
#include <iostream>
#include <stdio.h>

namespace gang {

GangDecoder::GangDecoder(const std::string& url,
		VideoFrameObserver* video_frame_observer,
		AudioFrameObserver* audio_frame_observer) :
		decoder_(::new_gang_decoder(url.c_str())), video_frame_observer_(
				video_frame_observer), audio_frame_observer_(
				audio_frame_observer), connected_(false) {
}

GangDecoder::~GangDecoder() {
	printf("GangDecoder::~GangDecoder\n");
	Stop();
	::free_gang_decode(decoder_);
}

void GangDecoder::Stop() {
	printf("GangDecoder::Stop\n");
	disconnect();
	rtc::Thread::Stop();
}

bool GangDecoder::Init() {
	if (::init_gang_decoder(decoder_) == 1) {
		return true;
	}
	return false;
}

void GangDecoder::GetBestFormat(int* width, int* height, int* fps) {
	*width = decoder_->width;
	*height = decoder_->height;
	*fps = decoder_->fps;
}

void GangDecoder::Run() {
	if (!connect()) {
		printf("failed to run\n");
		return;
	}
	printf("start to run\n");
	while (connected_ && !NextFrameLoop()) {
//		printf("running \n");
	}
	printf("stopping %s\n", connected_);
	disconnect();
}

bool GangDecoder::connect() {
	rtc::CritScope cs(&crit_);
	if (!connected_) {
		printf("::start_gang_decode start\n");
		connected_ = ::start_gang_decode(decoder_) == 1;
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
	uint8_t *data = 0;
	int size = 0;
	switch (::gang_decode_next_frame(decoder_, &data, &size)) {
	case 1:
		std::cout << "data size: " << size << std::endl;
		if (video_frame_observer_)
			video_frame_observer_->OnVideoFrame(static_cast<uint8*>(data),
					static_cast<uint32>(size));
		break;
	case 2:
		if (audio_frame_observer_)
			audio_frame_observer_->OnAudioFrame(reinterpret_cast<uint8*>(data),
					static_cast<uint32>(size));
		break;
	case -1:
		printf("EOF\n");
		return true;
	default:
		break;
	}
	return false;
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
