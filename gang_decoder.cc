#include "gang_decoder.h"

#include <memory>
#include "webrtc/base/bind.h"

#include "gang_spdlog_console.h"
#ifdef LOG_ERR
#undef LOG_ERR
#endif
#include "gang_decoder_impl.h"

namespace gang {

// Take from "talk/media/devices/yuvframescapturer.h"
class GangDecoder::GangThread: public rtc::Thread, public rtc::MessageHandler {
public:
	explicit GangThread(GangDecoder* dec) :
					dec_(dec),
					waiting_time_ms_(0),
					finished_(false) {
	}

	virtual ~GangThread() {
		Stop();
	}

	// Override virtual method of parent Thread. Context: Worker Thread.
	virtual void Run() {
		// Read the first frame and start the message pump. The pump runs until
		// Stop() is called externally or Quit() is called by OnMessage().
		if (dec_) {
			Thread::Run();
		}

		rtc::CritScope cs(&crit_);
		finished_ = true;
	}

	// Override virtual method of parent MessageHandler. Context: Worker Thread.
	virtual void OnMessage(rtc::Message* pmsg) {
		if (dec_) {
			switch (pmsg->message_id) {
			case NEXT:
				if (dec_->NextFrameLoop()) {
					PostDelayed(waiting_time_ms_, this, NEXT);
					dec_->connected_ = true;
				} else {
					dec_->connected_ = false;
				}
				break;
			case REC_ON:
				dec_->SetRecOn(static_cast<RecOnMsgData*>(pmsg->pdata)->data());
				break;
			case SHUTDOWN:
				dec_->Stop(true);
				break;
			default:
				break;
			}
		} else {
			Quit();
		}
	}

	// Check if Run() is finished.
	bool Finished() const {
		rtc::CritScope cs(&crit_);
		return finished_;
	}

private:
	GangDecoder* dec_;
	mutable rtc::CriticalSection crit_;
	int waiting_time_ms_;bool finished_;

	DISALLOW_COPY_AND_ASSIGN(GangThread);
};

GangDecoder::GangDecoder(const std::string& url, const std::string& rec_name, bool rec_enabled) :
				connected_(false),
				decoder_(::new_gang_decoder(url.c_str(), rec_name.c_str(), rec_enabled)),
				gang_thread_(NULL),
				video_frame_observer_(NULL),
				audio_frame_observer_(NULL) {
	SPDLOG_TRACE(console, "{}: url: {}, rec_name: {}", __FUNCTION__, url, rec_name)
}

GangDecoder::~GangDecoder() {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	if (gang_thread_) {
		gang_thread_->Post(gang_thread_, SHUTDOWN);
		gang_thread_->Stop();
		gang_thread_ = NULL;
	}
	::free_gang_decoder(decoder_);
}

bool GangDecoder::Init() {
	if (!decoder_) {
		return false;
	}
	gang_thread_ = new GangThread(this);
	return gang_thread_->Start() && !::init_gang_av_info(decoder_);
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

// Check if Run() is finished.
bool GangDecoder::IsRunning() {
	return connected_ && gang_thread_ && !gang_thread_->Finished();
}

bool GangDecoder::Start() {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	if (gang_thread_->Finished()) {
		return false;
	}
	if (!connected_) {
		connected_ = !::open_gang_decoder(decoder_);
	}
	if (!connected_) {
		return false;
	}
	gang_thread_->Clear(gang_thread_, NEXT);
	gang_thread_->PostDelayed(0, gang_thread_, NEXT);
	return true;
}

void GangDecoder::Stop(bool force) {
	SPDLOG_TRACE(console, "{}: {}", __FUNCTION__, force)
	if (!force && decoder_->rec_enabled) {
		return;
	}
	if (connected_) {
		connected_ = false;
		::flush_gang_rec_encoder(decoder_);
		::close_gang_decoder(decoder_);
	}
	gang_thread_->Clear(gang_thread_, NEXT);
}

// return true->continue, false->end
bool GangDecoder::NextFrameLoop() {
	switch (::gang_decode_next_frame(decoder_)) {
	case GANG_VIDEO_DATA:
		if (video_frame_observer_) {
			video_frame_observer_->OnGangFrame();
		}
		break;
	case GANG_AUDIO_DATA:
		if (audio_frame_observer_) {
			audio_frame_observer_->OnGangFrame();
		}
		break;
	case GANG_FITAL: // end loop
		SPDLOG_TRACE(console, "{}: {}", __FUNCTION__, "GANG_FITAL")
		return false;
	case GANG_ERROR_DATA: // ignore and next
		SPDLOG_TRACE(console, "{}: {}", __FUNCTION__, "GANG_ERROR_DATA")
		break;
	default: // unexpected, so end loop
		console->error() << "Unknow ret code from decoder!";
		return false;
	}
	return true;
}

// Do not call in the running thread
bool GangDecoder::SetVideoFrameObserver(GangFrameObserver* observer, uint8_t* buff) {
	return gang_thread_->Invoke<bool>(
			rtc::Bind(&GangDecoder::SetVideoObserver, this, observer, buff));
}

// Do not call in the running thread
bool GangDecoder::SetAudioFrameObserver(GangFrameObserver* observer, uint8_t* buff) {
	return gang_thread_->Invoke<bool>(
			rtc::Bind(&GangDecoder::SetAudioObserver, this, observer, buff));
}

bool GangDecoder::SetVideoObserver(GangFrameObserver* observer, uint8_t* buff) {
	video_frame_observer_ = observer;
	decoder_->video_buff = buff;
	if (observer) {
		return Start();
	}
	if (!audio_frame_observer_ && connected_) {
		Stop(false);
	}
	return false;
}

bool GangDecoder::SetAudioObserver(GangFrameObserver* observer, uint8_t* buff) {
	audio_frame_observer_ = observer;
	decoder_->audio_buff = buff;
	if (observer) {
		return Start();
	}
	if (!video_frame_observer_ && connected_) {
		Stop(false);
	}
	return false;
}

// TODO fix not to stop thread when toggle rec_enabled status.
// Do not call in the running thread
void GangDecoder::SetRecordEnabled(bool enabled) {
	gang_thread_->Post(gang_thread_, REC_ON, new RecOnMsgData(enabled));
}

void GangDecoder::SetRecOn(bool enabled) {
	if ((!decoder_->rec_enabled) == (!enabled)) {
		return;
	}
	Stop(true);
	decoder_->rec_enabled = enabled;
	if (enabled || video_frame_observer_ || audio_frame_observer_) {
		SPDLOG_TRACE(console, "{} {}", __FUNCTION__, "restart")
		Start();
	}
}

}  // namespace gang
