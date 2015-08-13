#include "gang_decoder.h"

#include <memory>
#include "webrtc/base/bind.h"

#include "gang_spdlog_console.h"
#include "gang_decoder_impl.h"

namespace gang {

using rtc::Bind;

// Take from "talk/media/devices/yuvframescapturer.h"
class GangDecoder::GangThread: public Thread, public rtc::MessageHandler {
public:
	explicit GangThread(GangDecoder* dec) :
					dec_(dec),
					waiting_time_ms_(0),
					finished_(false) {
	}

	virtual ~GangThread() {
		SPDLOG_TRACE(console, "{}", __FUNCTION__)
		Stop();
		SPDLOG_TRACE(console, "{} {}", __FUNCTION__, "ok")
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
				} else if (dec_->connected_) {
					dec_->Stop(true);
				}
				break;
			case REC_ON:
				dec_->SetRecOn(static_cast<RecOnMsgData*>(pmsg->pdata)->data());
				break;
			case VIDEO_OBSERVER: {
				rtc::scoped_ptr<ObserverMsgData> data(static_cast<ObserverMsgData*>(pmsg->pdata));
				dec_->SetVideoObserver(data->data()->observer, data->data()->buff);
				break;
			}
			case AUDIO_OBSERVER: {
				rtc::scoped_ptr<ObserverMsgData> data(static_cast<ObserverMsgData*>(pmsg->pdata));
				dec_->SetAudioObserver(data->data()->observer, data->data()->buff);
				break;
			}
			case START_REC:
				dec_->Start();
				break;
			case SHUTDOWN:
				SPDLOG_TRACE(console, "{}", __FUNCTION__)
				Clear(this);
				SPDLOG_TRACE(console, "{} {}", __FUNCTION__, "ok")
				break;
			default:
				console->error("{} {}", __FUNCTION__, "unexpected msg type");
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
	int waiting_time_ms_;bool finished_;
	mutable rtc::CriticalSection crit_;

	DISALLOW_COPY_AND_ASSIGN(GangThread);
};

GangDecoder::GangDecoder(const std::string& id, const std::string& url, const std::string& rec_name,
bool rec_enabled, Thread* worker_thread, StatusObserver* status_observer) :
				connected_(false),
				id_(id),
				decoder_(::new_gang_decoder(url.c_str(), rec_name.c_str(), rec_enabled)),
				gang_thread_(new GangThread(this)),
				worker_thread_(worker_thread),
				video_frame_observer_(NULL),
				audio_frame_observer_(NULL),
				status_observer_(status_observer) {
	gang_thread_->Start();
	SPDLOG_TRACE(console, "{}: url: {}, rec_name: {}", __FUNCTION__, url, rec_name)
}

GangDecoder::~GangDecoder() {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	if (gang_thread_) {
		gang_thread_->Post(gang_thread_, SHUTDOWN);
		delete gang_thread_;
		gang_thread_ = NULL;
	}
	if (decoder_) {
		stop();
		::free_gang_decoder(decoder_);
		decoder_ = NULL;
	}
	SPDLOG_TRACE(console, "{} {}", __FUNCTION__, "ok")
}

bool GangDecoder::Init() {
	if (!decoder_ || !worker_thread_) {
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

bool GangDecoder::IsRunning() {
	return gang_thread_ && !gang_thread_->Finished();
}

void GangDecoder::stop() {
	connected_ = false;
	::flush_gang_rec_encoder(decoder_);
	::close_gang_decoder(decoder_);
	SPDLOG_TRACE(console, "{}: {}", __FUNCTION__, "shutdown")
}

bool GangDecoder::Start() {
	DCHECK(gang_thread_->IsCurrent());
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	if (gang_thread_->Finished()) {
		return false;
	}
	if (!connected_) {
		connected_ = !::open_gang_decoder(decoder_);
		if (connected_) {
			SendStatus(Alive);
		} else {
			SendStatus(Dead);
		}
	}
	if (!connected_) {
		return false;
	}
	gang_thread_->Clear(gang_thread_, NEXT);
	gang_thread_->PostDelayed(0, gang_thread_, NEXT);
	return true;
}

void GangDecoder::Stop(bool force) {
	DCHECK(gang_thread_->IsCurrent());
	SPDLOG_TRACE(console, "{}: {}", __FUNCTION__, force)
	if (!force && decoder_->rec_enabled) {
		return;
	}
	stop();
	gang_thread_->Clear(gang_thread_, NEXT);
}

// return true->continue, false->end
bool GangDecoder::NextFrameLoop() {
	DCHECK(gang_thread_->IsCurrent());
	switch (::gang_decode_next_frame(decoder_)) {
	case GANG_VIDEO_DATA:
		if (video_frame_observer_) {
			worker_thread_->Invoke<void>(
					Bind(&GangFrameObserver::OnGangFrame, video_frame_observer_));
		}
		break;
	case GANG_AUDIO_DATA:
		if (audio_frame_observer_) {
			worker_thread_->Invoke<void>(
					Bind(&GangFrameObserver::OnGangFrame, audio_frame_observer_));
		}
		break;
	case GANG_FITAL: // end loop
		SPDLOG_TRACE(console, "{}: {}", __FUNCTION__, "GANG_FITAL")
		SendStatus(Dead);
		return false;
	case GANG_ERROR_DATA: // ignore and next
		break;
	default: // unexpected, so end loop
		console->error() << "Unknow ret code from decoder!";
		SendStatus(Dead);
		return false;
	}
	return true;
}

// Called by webrtc worker thread
void GangDecoder::SetVideoFrameObserver(GangFrameObserver* observer, uint8_t* buff) {
//	bool previous = Thread::Current()->SetAllowBlockingCalls(true);
//	bool ok = gang_thread_->Invoke<bool>(
//			Bind(&GangDecoder::SetVideoObserver, this, observer, buff));
//	Thread::Current()->SetAllowBlockingCalls(previous);
//	return ok;
	gang_thread_->Post(
			gang_thread_,
			VIDEO_OBSERVER,
			new ObserverMsgData(new Observer(observer, buff)));
}

// Called by webrtc worker thread
void GangDecoder::SetAudioFrameObserver(GangFrameObserver* observer, uint8_t* buff) {
	gang_thread_->Post(
			gang_thread_,
			AUDIO_OBSERVER,
			new ObserverMsgData(new Observer(observer, buff)));
}

bool GangDecoder::SetVideoObserver(GangFrameObserver* observer, uint8_t* buff) {
	DCHECK(gang_thread_->IsCurrent());
	video_frame_observer_ = observer;
	decoder_->video_buff = buff;
	if (observer) {
		return Start();
	}
	if (!audio_frame_observer_) {
		Stop(false);
	}
	return false;
}

bool GangDecoder::SetAudioObserver(GangFrameObserver* observer, uint8_t* buff) {
	DCHECK(gang_thread_->IsCurrent());
	audio_frame_observer_ = observer;
	decoder_->audio_buff = buff;
	if (observer) {
		return Start();
	}
	if (!video_frame_observer_) {
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
	DCHECK(gang_thread_->IsCurrent());
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

void GangDecoder::SendStatus(GangStatus status) {
	if (status_observer_) {
		status_observer_->OnStatusChange(id_, status);
	}
}

void GangDecoder::StartRec() {
	if (decoder_->rec_enabled) {
		gang_thread_->Post(gang_thread_, START_REC);
	}
}

}  // namespace gang
