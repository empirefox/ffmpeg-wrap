#ifndef GANGVIDEOCAPTURER_H_
#define GANGVIDEOCAPTURER_H_

#include <string>

#include "talk/media/base/videocapturer.h"
#include "webrtc/base/stringutils.h"
#include "gang_decoder.h"

using cricket::VideoCapturer;
using cricket::VideoFormat;
using cricket::CaptureState;
using cricket::CapturedFrame;
using cricket::Device;

namespace gang {

// Simulated video capturer that reads frames from a url.
class GangVideoCapturer: public VideoCapturer, public VideoFrameObserver {
public:
	GangVideoCapturer(const std::string& url);
	virtual ~GangVideoCapturer();

	// Determines if the given device is actually a video file, to be captured
	// with a GangVideoCapturer.
	static bool IsGangVideoCapturerDevice(const Device& device) {
		return rtc::starts_with(device.id.c_str(), kVideoGangDevicePrefix);
	}

	// Creates a fake device for the given filename.
	static Device CreateGangVideoCapturerDevice(const std::string& url) {
		std::stringstream id;
		id << kVideoGangDevicePrefix << url;
		return Device(url, id.str());
	}

	void Init(GangDecoder* gang_thread);

	// Override virtual methods of parent class VideoCapturer.
	virtual CaptureState Start(const VideoFormat& capture_format);
	virtual void Stop();
	virtual bool IsRunning();
	virtual bool IsScreencast() const {
		return false;
	}

	// Implements VideoFrameObserver
	// data uint8*
	virtual void OnVideoFrame(void* data, uint32 size);

protected:
	// Override virtual methods of parent class VideoCapturer.
	virtual bool GetPreferredFourccs(std::vector<uint32>* fourccs);

	// Return the CapturedFrame - useful for extracting contents after reading
	// a frame. Should be used only while still reading a file (i.e. only while
	// the CapturedFrame object still exists).
	const CapturedFrame* frame() const {
		return &captured_frame_;
	}

private:

	static const char* kVideoGangDevicePrefix;
	CapturedFrame captured_frame_;
	GangDecoder* gang_thread_;
	int64 start_time_ns_;  // Time when the file video capturer starts.

	DISALLOW_COPY_AND_ASSIGN(GangVideoCapturer);
};

}  // namespace gang

#endif  // GANGVIDEOCAPTURER_H_
