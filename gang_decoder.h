#ifndef GANG_GANG_DECODER_HH
#define GANG_GANG_DECODER_HH

#include "webrtc/base/basictypes.h"
#include "webrtc/base/constructormagic.h"
#include "webrtc/base/criticalsection.h"
#include "webrtc/base/thread.h"
#include "talk/media/base/videocapturer.h"
#include "gang_decoder.h"
#include "gang_decoder_impl.h"

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
	// signal data int16_t*
	virtual void OnAudioFrame(void* data, uint32_t nSamples) = 0;
protected:
	~AudioFrameObserver() {
	}
};

class GangDecoder: public rtc::Thread {
public:
	explicit GangDecoder(
			const std::string& url,
			VideoFrameObserver* video_frame_observer = NULL,
			AudioFrameObserver* audio_frame_observer = NULL);
	~GangDecoder();

	bool Init();

	virtual void Run() override;
	virtual void Stop() override;
	bool Connected();

	bool NextFrameLoop();

	void SetVideoFrameObserver(VideoFrameObserver* video_frame_observer_);
	void SetAudioFrameObserver(AudioFrameObserver* audio_frame_observer_);
	// TODO change to GetVideoInfo
	void GetBestFormat(int* width, int* height, int* fps);
	void GetAudioInfo(
			uint32_t* sample_rate,
			uint8_t* channels);
private:
	bool connect();
	void disconnect();
	gang_decoder* decoder_;
	VideoFrameObserver* video_frame_observer_;
	AudioFrameObserver* audio_frame_observer_;

	mutable rtc::CriticalSection crit_;
	bool connected_;
	int escape_;
	int escaped_;

	DISALLOW_COPY_AND_ASSIGN(GangDecoder);
};

}
// namespace gang

#endif // GANG_GANG_DECODER_HH
