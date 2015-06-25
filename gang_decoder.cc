#include "gang_decoder.h"

#include <memory>

#include "gang_spdlog_console.h"
#ifndef LOG_ERR
#undef LOG_ERR
#endif
#include "gang_decoder_impl.h"

namespace gang {

GangDecoder::GangDecoder(const std::string& url, const std::string& rec_name, bool rec_enabled) :
				decoder_(::new_gang_decoder(url.c_str(), rec_name.c_str(), rec_enabled)),
				video_frame_observer_(NULL),
				audio_frame_observer_(NULL),
				connected_(false),
				rec_enabled_(rec_enabled) {
	SPDLOG_TRACE(console, "url: {}, rec_name: {}", url, rec_name)
}

GangDecoder::~GangDecoder() {
	SPDLOG_TRACE(console)
	Stop();
	::free_gang_decoder(decoder_);
}

// Wait for loop end
void GangDecoder::Stop() {
	SPDLOG_TRACE(console)
	{
		rtc::CritScope cs(&crit_);
		stop(true);
	}
	rtc::Thread::Stop();
}

bool GangDecoder::Init() {
	if (!decoder_) {
		return false;
	}
	return !::init_gang_av_info(decoder_);
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
	if (connect()) {
		SPDLOG_TRACE(console, "connected")
		while (connected_ && nextFrameLoop()) {
		}
	}
	SPDLOG_TRACE(console, "before disconnect")
	disconnect();
}

// Check if Run() is finished.
bool GangDecoder::Connected() {
	rtc::CritScope cs(&crit_);
	return connected_;
}

bool GangDecoder::connect() {
	SPDLOG_TRACE(console)
	rtc::CritScope cs(&crit_);
	if (!connected_) {
		connected_ = !::open_gang_decoder(decoder_);
	}
	return connected_;
}

void GangDecoder::disconnect() {
	SPDLOG_TRACE(console)
	Stop();
	rtc::CritScope cs(&crit_);
	::flush_gang_rec_encoder(decoder_);
	::close_gang_decoder(decoder_);
}

// Just end next loop
void GangDecoder::stop(bool force) {
	SPDLOG_DEBUG(console, "Stop")
	if (!force && rec_enabled_) {
		return;
	}
	connected_ = false;
}

// return true->continue, false->end
bool GangDecoder::nextFrameLoop() {
	uint8_t *data = NULL;
	// or nSamples with audio data
	int size = 0;

	rtc::CritScope cs(&crit_);
	switch (::gang_decode_next_frame(
			decoder_,
			(video_frame_observer_ || audio_frame_observer_) ? &data : NULL,
			&size)) {
	case GANG_VIDEO_DATA:
		if (video_frame_observer_) {
			video_frame_observer_->OnVideoFrame(
					reinterpret_cast<void*>(data),
					static_cast<uint32>(size));
			break;
		}
		::free(data);
		break;
	case GANG_AUDIO_DATA:
		if (audio_frame_observer_
				&& audio_frame_observer_->OnAudioFrame(data, static_cast<uint32_t>(size))) {
			break;
		}
		::free(data);
		break;
	case GANG_FITAL: // end loop
		SPDLOG_DEBUG(console, "GANG_EOF")
		return false;
	case GANG_ERROR_DATA: // ignore and next
		break;
	default: // unexpected, so end loop
		console->error() << "Unknow ret code from decoder!";
		return false;
	}
	return true;
}

// Do not call in the running thread
bool GangDecoder::SetVideoFrameObserver(VideoFrameObserver* video_frame_observer) {
	rtc::CritScope cs(&crit_);
	video_frame_observer_ = video_frame_observer;
	if (video_frame_observer_) {
		SPDLOG_DEBUG(console, "Start")
		return connected_ || Start();
	}
	if (!audio_frame_observer_ && connected_) {
		stop(false);
	}
	return false;
}

// Do not call in the running thread
bool GangDecoder::SetAudioFrameObserver(AudioFrameObserver* audio_frame_observer) {
	rtc::CritScope cs(&crit_);
	audio_frame_observer_ = audio_frame_observer;
	if (audio_frame_observer_) {
		SPDLOG_DEBUG(console, "Start")
		return connected_ || Start();
	}
	if (!video_frame_observer_ && connected_) {
		stop(false);
	}
	return false;
}

// TODO fix not to stop thread when toggle rec_enabled status.
// Do not call in the running thread
void GangDecoder::SetRecordEnabled(bool enabled) {
	{
		rtc::CritScope cs(&crit_);
		if (rec_enabled_ == enabled) {
			return;
		}
		stop(true);
	}
	rtc::Thread::Stop();
	rtc::CritScope cs(&crit_);
	rec_enabled_ = enabled;
	decoder_->rec_enabled = enabled;
	if (rec_enabled_ || video_frame_observer_ || audio_frame_observer_) {
		Start();
	}
}

}  // namespace gang
