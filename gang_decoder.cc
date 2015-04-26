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

void GangDecoder::GetAudioInfo(int* sample_rate, int* channels, int* bytesPerSample){
	*sample_rate = decoder_->sample_rate;
	*channels = decoder_->channels;
	*bytesPerSample = decoder_->bytesPerSample;
}

void GangDecoder::Run() {
	if (!connect()) {
		return;
	}
	while (connected_ && !NextFrameLoop()) {
	}
	disconnect();
}

bool GangDecoder::connect() {
	rtc::CritScope cs(&crit_);
	if (!connected_) {
		connected_ = ::start_gang_decode(decoder_) == 1;
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
	void *data = 0;
	// nSamples
	int size = 0;
	switch (::gang_decode_next_frame(decoder_, &data, &size)) {
	case 1:
		if (video_frame_observer_)
			video_frame_observer_->OnVideoFrame(data,
					static_cast<uint32>(size));
		break;
	case 2:
		if (audio_frame_observer_)
			audio_frame_observer_->OnAudioFrame(data,
					static_cast<uint32_t>(size));
		else
			free(data);
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
