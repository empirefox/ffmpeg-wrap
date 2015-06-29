#ifndef GANGVIDEOCAPTURER_H_
#define GANGVIDEOCAPTURER_H_

#include <memory>
#include "talk/media/base/videocapturer.h"
#include "gang_decoder.h"

using std::shared_ptr;
using cricket::VideoCapturer;
using cricket::VideoFormat;
using cricket::CaptureState;
using cricket::CapturedFrame;

namespace gang {

// Simulated video capturer that reads frames from a url.
class GangVideoCapturer: public VideoCapturer, public GangFrameObserver {
public:
	virtual ~GangVideoCapturer();

	static GangVideoCapturer* Create(shared_ptr<GangDecoder> gang);

	// Override virtual methods of parent class VideoCapturer.
	virtual CaptureState Start(const VideoFormat& capture_format);
	virtual void Stop();
	virtual bool IsRunning();
	virtual bool IsScreencast() const {
		return false;
	}

	// Implements VideoFrameObserver
	// data uint8*
	virtual void OnGangFrame();

protected:
	GangVideoCapturer(shared_ptr<GangDecoder> gang);
	void Initialize();
	// Override virtual methods of parent class VideoCapturer.
	virtual bool GetPreferredFourccs(std::vector<uint32>* fourccs);

private:
	CapturedFrame captured_frame_;
	shared_ptr<GangDecoder> gang_;
	int64 start_time_ns_;  // Time when the capturer starts.

	DISALLOW_COPY_AND_ASSIGN(GangVideoCapturer);
};

}  // namespace gang

#endif  // GANGVIDEOCAPTURER_H_
