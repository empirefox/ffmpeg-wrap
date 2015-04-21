#include "gangvideocapturer.h"

namespace gang {

const char* GangVideoCapturer::kVideoGangDevicePrefix = "gang:";
static const int64 kNumNanoSecsPerMilliSec = 1000000;

GangVideoCapturer::GangVideoCapturer(const std::string& url) :
		gang_thread_(NULL), start_time_ns_(0) {
	SetId(url);
}

GangVideoCapturer::~GangVideoCapturer() {
	if (gang_thread_) {
		gang_thread_->Stop();
		gang_thread_ = NULL;
	}
	Stop();
	delete[] static_cast<uint8*>(captured_frame_.data);
}

void GangVideoCapturer::Init(GangDecoder* gang_thread) {
	gang_thread_ = gang_thread;
	int width;
	int height;
	int fps;
	gang_thread_->GetBestFormat(&width, &height, &fps);

	captured_frame_.fourcc = cricket::FOURCC_I420;
	captured_frame_.pixel_height = 1;
	captured_frame_.pixel_width = 1;
	captured_frame_.width = width;
	captured_frame_.height = height;

	// Enumerate the supported formats. We have only one supported format. We set
	// the frame interval to kMinimumInterval here. In Start(), if the capture
	// format's interval is greater than kMinimumInterval, we use the interval;
	// otherwise, we use the timestamp in the file to control the interval.
	// TODO change supper fps to that from stream
	cricket::VideoFormat format(width, height,
			cricket::VideoFormat::kMinimumInterval, cricket::FOURCC_I420);
	std::vector<VideoFormat> supported;
	supported.push_back(format);
	SetSupportedFormats(supported);

	// TODO(wuwang): Design an E2E integration test for video adaptation,
	// then remove the below call to disable the video adapter.
	set_enable_video_adapter(false);
}

CaptureState GangVideoCapturer::Start(const VideoFormat& capture_format) {
	if (IsRunning()) {
		return cricket::CS_FAILED;
	}
	SetCaptureFormat(&capture_format);

	start_time_ns_ = kNumNanoSecsPerMilliSec * static_cast<int64>(rtc::Time());

	if (gang_thread_ && gang_thread_->Start()) {
		return cricket::CS_RUNNING;
	}
	return cricket::CS_FAILED;
}

void GangVideoCapturer::Stop() {
	SetCaptureFormat(NULL);
}

bool GangVideoCapturer::IsRunning() {
	return gang_thread_ && gang_thread_->Connected();
}

void GangVideoCapturer::OnVideoFrame(uint8* data, uint32 size) {
	uint32 start_read_time_ms = rtc::Time();
	captured_frame_.data_size = size;
	captured_frame_.data = reinterpret_cast<void*>(data);

	captured_frame_.time_stamp = kNumNanoSecsPerMilliSec
			* static_cast<int64>(start_read_time_ms);
	captured_frame_.elapsed_time = captured_frame_.time_stamp - start_time_ns_;

	SignalFrameCaptured(this, &captured_frame_);
}

bool GangVideoCapturer::GetPreferredFourccs(std::vector<uint32>* fourccs) {
	if (!fourccs) {
		return false;
	}

	fourccs->push_back(GetSupportedFormats()->at(0).fourcc);
	return true;
}

}  // namespace gang
