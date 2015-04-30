#include  "gang_decoder.h"
#include  "gang_decoder_impl.h"
#include <iostream>
#include <stdio.h>

namespace gang {

GangDecoder::GangDecoder(
		const std::string& url,
		VideoFrameObserver* video_frame_observer,
		AudioFrameObserver* audio_frame_observer) :
				decoder_(::new_gang_decoder(url.c_str())),
				video_frame_observer_(video_frame_observer),
				audio_frame_observer_(audio_frame_observer),
				connected_(false),
				escape_(100),
				escaped_(0) {
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
	printf("GangDecoder::Init %s\n", ok ? "true" : "false");
	return ok;
}

void GangDecoder::GetBestFormat(int* width, int* height, int* fps) {
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
	while (connected_ && !NextFrameLoop()) {
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
		::close_gang_decoder(decoder_);
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
			video_frame_observer_->OnVideoFrame(
					data,
					static_cast<uint32>(size));
		break;
	case 2:
		if (audio_frame_observer_) {
			++escaped_;
			if (escaped_ > escape_) {
				audio_frame_observer_->OnAudioFrame(
						data,
						static_cast<uint32_t>(size));
			} else {
				free(data);
			}
		} else
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
