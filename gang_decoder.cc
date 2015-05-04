#include "gang_decoder.h"

#include <memory>

#include "gang_decoder_impl.h"
#include "gang_spdlog_console.h"

namespace gang {

GangDecoder::GangDecoder(const std::string& url) :
				decoder_(::new_gang_decoder(url.c_str())),
				video_frame_observer_(NULL),
				audio_frame_observer_(NULL),
				connected_(false) {
	SPDLOG_TRACE(console);
}

GangDecoder::~GangDecoder() {
	SPDLOG_TRACE(console);
	Stop();
	::free_gang_decoder(decoder_);
}

void GangDecoder::Stop() {
	SPDLOG_TRACE(console);
	disconnect();
	rtc::Thread::Stop();
}

bool GangDecoder::Init() {
	bool ok = !::open_gang_decoder(decoder_);
	::close_gang_decoder(decoder_);
	SPDLOG_TRACE(console, ok ? "ok" : "failed");
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
	SPDLOG_TRACE(console);
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
	SPDLOG_TRACE(console);
	rtc::CritScope cs(&crit_);
	if (connected_) {
		stop();
	}
}

void GangDecoder::stop() {
	SPDLOG_DEBUG(console, "Stop");
	::close_gang_decoder(decoder_);
	connected_ = false;
}

// return true->continue, false->end
bool GangDecoder::nextFrameLoop() {
	uint8_t *data = 0;
	// or nSamples with audio data
	int size = 0;
	switch (::gang_decode_next_frame(decoder_, &data, &size)) {
	case GANG_VIDEO_DATA:
		if (video_frame_observer_) {
			video_frame_observer_->OnVideoFrame(
					reinterpret_cast<void*>(data),
					static_cast<uint32>(size));
		} else {
			free(data);
		}
		break;
	case GANG_AUDIO_DATA:
		if (audio_frame_observer_
				&& audio_frame_observer_->OnAudioFrame(
						data,
						static_cast<uint32_t>(size))) {
			break;
		}
		free(data);
		break;
	case GANG_EOF: // end loop
		SPDLOG_DEBUG(console, "GANG_EOF");
		return false;
	case GANG_ERROR_DATA: // next
		SPDLOG_TRACE(console);
		break;
	default: // unexpected, so end loop
		console->error() << "Unknow ret code from decoder!";
		return false;
	}
	return true;
}

// Do not call in the running thread
bool GangDecoder::SetVideoFrameObserver(
		VideoFrameObserver* video_frame_observer) {
	rtc::CritScope cs(&crit_);
	video_frame_observer_ = video_frame_observer;
	if (video_frame_observer_) {
		SPDLOG_DEBUG(console, "Start");
		return connected_ || Start();
	}
	if (!audio_frame_observer_ && connected_) {
		stop();
	}
	return false;
}

// Do not call in the running thread
bool GangDecoder::SetAudioFrameObserver(
		AudioFrameObserver* audio_frame_observer) {
	rtc::CritScope cs(&crit_);
	audio_frame_observer_ = audio_frame_observer;
	if (audio_frame_observer_) {
		SPDLOG_DEBUG(console, "Start");
		return connected_ || Start();
	}
	if (!video_frame_observer_ && connected_) {
		stop();
	}
	return false;
}

}  // namespace gang
