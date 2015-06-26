#ifndef GANG_DECODER_H_
#define GANG_DECODER_H_

#include "webrtc/base/basictypes.h"
#include "webrtc/base/constructormagic.h"
#include "webrtc/base/criticalsection.h"
#include "webrtc/base/thread.h"
#include "talk/media/base/videocapturer.h"

#include "gang_dec.h"

namespace gang {

// need be shared_ptr
class VideoFrameObserver {
public:
	// signal data uint8*
	// see "talk/media/webrtc/webrtcvideoframe.h"
	virtual void OnVideoFrame(void* data, uint32 size) = 0;
protected:
	~VideoFrameObserver() {
	}
};

// need be shared_ptr
class AudioFrameObserver {
public:
	// return true if data has been freed
	virtual void OnAudioFrame(uint8_t* data, uint32_t nSamples) = 0;
protected:
	~AudioFrameObserver() {
	}
};

class GangDecoder: public rtc::Thread {
public:
	explicit GangDecoder(const std::string& url, const std::string& rec_name, bool rec_enabled);
	~GangDecoder();

	bool Init();

	virtual void Run() override;
	virtual void Stop() override;
	bool Connected();

	bool IsVideoAvailable();
	bool IsAudioAvailable();

	bool SetVideoFrameObserver(VideoFrameObserver* video_frame_observer_);
	bool SetAudioFrameObserver(AudioFrameObserver* audio_frame_observer_);

	void GetVideoInfo(int* width, int* height, int* fps);
	void GetAudioInfo(uint32_t* sample_rate, uint8_t* channels);

	void SetRecordEnabled(bool enabled);

private:
	bool nextFrameLoop();
	bool connect();
	void disconnect();
	void stop(bool force);

	gang_decoder* decoder_;
	VideoFrameObserver* video_frame_observer_;
	AudioFrameObserver* audio_frame_observer_;

	mutable rtc::CriticalSection crit_;
	bool connected_;
	bool rec_enabled_;

	DISALLOW_COPY_AND_ASSIGN(GangDecoder);
};

}
// namespace gang

#endif // GANG_DECODER_H_
