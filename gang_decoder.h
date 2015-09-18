#ifndef GANG_DECODER_H_
#define GANG_DECODER_H_

#include "webrtc/base/basictypes.h"
#include "webrtc/base/constructormagic.h"
#include "webrtc/base/criticalsection.h"
#include "webrtc/base/thread.h"
#include "talk/media/base/videocapturer.h"

#include "gang_dec.h"

namespace gang {

using rtc::Thread;

typedef enum GangStatus {
	Alive, Dead
} GangStatus;

// Thread safe is the user's responsibility
class StatusObserver {
public:
	virtual void OnStatusChange(const std::string& id, GangStatus status) = 0;
	virtual ~StatusObserver() {
	}
};

// need be shared_ptr
class GangFrameObserver {
public:
	// see "talk/media/webrtc/webrtcvideoframe.h"
	virtual void OnGangFrame() = 0;
	virtual void OnVideoStarted(bool success) {
	}
	virtual void OnVideoStopped() {
	}
	virtual ~GangFrameObserver() {
	}
};

class Observer {
public:
	Observer(GangFrameObserver* _observer, uint8_t* _buff) :
					observer(_observer),
					buff(_buff) {
	}
	GangFrameObserver* observer;
	uint8_t* buff;
};

typedef rtc::ScopedMessageData<Observer> ObserverMsgData;
typedef rtc::TypedMessageData<bool> RecOnMsgData;

class GangDecoder {
public:
	enum {
		NEXT, REC_ON, START_REC, SHUTDOWN, VIDEO_START, VIDEO_STOP, AUDIO_OBSERVER
	};

	explicit GangDecoder(
			const std::string& id,
			const std::string& url,
			const std::string& rec_name,
			bool rec_enabled,
			bool audio_off,
			Thread* worker_thread,
			StatusObserver* status_observer);
	~GangDecoder();

	bool Init();

	// only in worker thread
	bool Start();
	void Stop(bool force);

	bool IsVideoAvailable();
	bool IsAudioAvailable();

	void StartVideoCapture(GangFrameObserver* observer, uint8_t* buff);
	void StopVideoCapture(GangFrameObserver* observer);
	void SetAudioFrameObserver(GangFrameObserver* observer, uint8_t* buff);

	void GetVideoInfo(int* width, int* height, int* fps, uint32* buf_size);
	void GetAudioInfo(uint32_t* sample_rate, uint8_t* channels);

	void SetRecordEnabled(bool enabled);
	void SendStatus(GangStatus status);

	// these can be called outside gang thread.
	// but only at beginning or end.
	void StartRec();
	void Shutdown();

protected:
	void stop();
	bool NextFrameLoop();
	void SetRecOn(bool enabled);

	// only in worker thread
	void StartVideoCapture_g(GangFrameObserver* observer, uint8_t* buff);
	void StopVideoCapture_g(GangFrameObserver* observer);
	void SetAudioObserver_g(GangFrameObserver* observer, uint8_t* buff);

	bool connected_;

private:
	class GangThread; // Forward declaration, defined in .cc.

	const std::string id_;
	gang_decoder* decoder_;
	GangThread* gang_thread_;
	Thread* worker_thread_;
	GangFrameObserver* video_frame_observer_;
	GangFrameObserver* audio_frame_observer_;
	StatusObserver* status_observer_;

	mutable rtc::CriticalSection crit_;

	RTC_DISALLOW_COPY_AND_ASSIGN(GangDecoder);
};

}
// namespace gang

#endif // GANG_DECODER_H_
