#ifndef GANGVIDEOCAPTURER_H_
#define GANGVIDEOCAPTURER_H_

#include <string>

#include "talk/media/base/videocapturer.h"

using namespace cricket;

// Simulated video capturer that periodically reads frames from a file.
class GangVideoCapturer: public VideoCapturer {
public:
	static const int kForever = -1;

	GangVideoCapturer();
	virtual ~GangVideoCapturer();

	// Determines if the given device is actually a video file, to be captured
	// with a GangVideoCapturer.
	static bool IsGangVideoCapturerDevice(const Device& device) {
		return rtc::starts_with(device.id.c_str(), kVideoFileDevicePrefix);
	}

	// Creates a fake device for the given filename.
	static Device CreateGangVideoCapturerDevice(const std::string& filename) {
		std::stringstream id;
		id << kVideoFileDevicePrefix << filename;
		return Device(filename, id.str());
	}

	// Initializes the capturer with the given file.
	bool Init(const std::string& filename);

	// Initializes the capturer with the given device. This should only be used
	// if IsFileVideoCapturerDevice returned true for the given device.
	bool Init(const Device& device);

	// Override virtual methods of parent class VideoCapturer.
	virtual CaptureState Start(const VideoFormat& capture_format);
	virtual void Stop();
	virtual bool IsRunning();
	virtual bool IsScreencast() const {
		return false;
	}

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

	static const char* kVideoFileDevicePrefix;
	CapturedFrame captured_frame_;
	// The number of bytes allocated buffer for captured_frame_.data.
	uint32 frame_buffer_size_;
	int64 start_time_ns_;  // Time when the file video capturer starts.

	DISALLOW_COPY_AND_ASSIGN(GangVideoCapturer);
};

#endif  // GANGVIDEOCAPTURER_H_
