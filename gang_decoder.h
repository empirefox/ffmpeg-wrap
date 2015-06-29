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

// need be shared_ptr
class GangFrameObserver {
public:
	// see "talk/media/webrtc/webrtcvideoframe.h"
	virtual void OnGangFrame() = 0;
protected:
	~GangFrameObserver() {
	}
};

//class Observer {
//public:
//	Observer(GangFrameObserver* _observer, uint8_t* _buff) :
//					observer(_observer),
//					buff(_buff) {
//	}
//	GangFrameObserver* observer;
//	uint8_t* buff;
//};
//
//typedef rtc::ScopedMessageData<Observer> ObserverMsgData;
typedef rtc::TypedMessageData<bool> RecOnMsgData;

class GangDecoder {
public:
	enum {
		NEXT, REC_ON, SHUTDOWN, VIDEO_OBSERVER, AUDIO_OBSERVER
	};

	explicit GangDecoder(
			const std::string& url,
			const std::string& rec_name,
			bool rec_enabled,
			Thread* worker_thread);
	~GangDecoder();

	bool Init();
	bool IsRunning();

	// only in worker thread
	bool Start();
	void Stop(bool force);

	bool IsVideoAvailable();
	bool IsAudioAvailable();

	bool SetVideoFrameObserver(GangFrameObserver* observer, uint8_t* buff);
	bool SetAudioFrameObserver(GangFrameObserver* observer, uint8_t* buff);

	void GetVideoInfo(int* width, int* height, int* fps);
	void GetAudioInfo(uint32_t* sample_rate, uint8_t* channels);

	void SetRecordEnabled(bool enabled);

protected:
	bool NextFrameLoop();
	void SetRecOn(bool enabled);;
	// only in worker thread
	bool SetVideoObserver(GangFrameObserver* observer, uint8_t* buff);
	bool SetAudioObserver(GangFrameObserver* observer, uint8_t* buff);

	bool connected_;

private:
	void stop();

	class GangThread; // Forward declaration, defined in .cc.

	gang_decoder* decoder_;
	GangThread* gang_thread_;
	Thread* worker_thread_;
	GangFrameObserver* video_frame_observer_;
	GangFrameObserver* audio_frame_observer_;

	DISALLOW_COPY_AND_ASSIGN(GangDecoder);
};

}
// namespace gang

#endif // GANG_DECODER_H_
