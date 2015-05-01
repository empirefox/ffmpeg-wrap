#include "gang_decoder.h"
#include "gang_decoder_impl.h"
#include <iostream>
#include <stdio.h>

namespace gang {

GangDecoder::GangDecoder(const std::string& url) :
				decoder_(::new_gang_decoder(url.c_str())),
				video_frame_observer_(NULL),
				audio_frame_observer_(NULL),
				connected_(false) {
}

GangDecoder::~GangDecoder() {
	printf("GangDecoder::~GangDecoder\n");
	Stop();
	::free_gang_decoder(decoder_);
}

void GangDecoder::Stop() {
	printf("GangDecoder::Stop\n");
	disconnect();
	rtc::Thread::Stop();
}

bool GangDecoder::Init() {
	bool ok = !::open_gang_decoder(decoder_);
	::close_gang_decoder(decoder_);
	printf("GangDecoder::Init %s\n", ok ? "ok" : "failed");
	return ok;
}

bool GangDecoder::IsVideoAvailable() {
	return !decoder_->no_video;
}

bool GangDecoder::IsAudioAvailable() {
	return !decoder_->no_audio;
}

void GangDecoder::GetVideoInfo(int* width, int* height, int* fps) {
	*width = decoder_->width;
	*height = decoder_->height;
	*fps = decoder_->fps;
}

void GangDecoder::GetAudioInfo(uint32_t* sample_rate, uint8_t* channels) {
	*sample_rate = static_cast<uint32_t>(decoder_->sample_rate);
	*channels = static_cast<uint8_t>(decoder_->channels);
}

void GangDecoder::Run() {
	if (!connect()) {
		return;
	}
	while (connected_ && nextFrameLoop()) {
	}
	disconnect();
}

bool GangDecoder::connect() {
	rtc::CritScope cs(&crit_);
	if (!connected_) {
		connected_ = !::open_gang_decoder(decoder_);
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
		stop();
	}
}

void GangDecoder::stop() {
	::close_gang_decoder(decoder_);
	connected_ = false;
}

// return true->continue, false->end
bool GangDecoder::nextFrameLoop() {
	void *data = 0;
	// or nSamples with audio data
	int size = 0;
	switch (::gang_decode_next_frame(decoder_, &data, &size)) {
	case GANG_VIDEO_DATA:
		if (video_frame_observer_)
			video_frame_observer_->OnVideoFrame(
					data,
					static_cast<uint32>(size));
		break;
	case GANG_AUDIO_DATA:
		if (audio_frame_observer_) {
			audio_frame_observer_->OnAudioFrame(
					data,
					static_cast<uint32_t>(size));
		} else
			free(data);
		break;
	case GANG_EOF: // end loop
		return false;
	case GANG_ERROR_DATA: // next
		break;
	default: // unexpected, so end loop
		return false;
	}
	return true;
}

// Do not call in the running thread
bool GangDecoder::SetVideoFrameObserver(
		VideoFrameObserver* video_frame_observer) {
	rtc::CritScope cs(&crit_);
	video_frame_observer_ = video_frame_observer;
	return startOrStop();
}

// Do not call in the running thread
bool GangDecoder::SetAudioFrameObserver(
		AudioFrameObserver* audio_frame_observer) {
	rtc::CritScope cs(&crit_);
	audio_frame_observer_ = audio_frame_observer;
	return startOrStop();
}

// return started or start action ok(will be started)
bool GangDecoder::startOrStop() {
	if ((video_frame_observer_ || audio_frame_observer_) && !connected_) {
		return Start();
	} else if (!video_frame_observer_ && !audio_frame_observer_ && connected_) {
		// TODO check if recording here
		stop();
	}
	return connected_;
}

}  // namespace gang
