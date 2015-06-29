#include "gangvideocapturer.h"

#include "gang_spdlog_console.h"

namespace gang {

GangVideoCapturer::GangVideoCapturer(shared_ptr<GangDecoder> gang) :
				gang_(gang),
				start_time_ns_(0),
				capture_(false) {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
}

GangVideoCapturer::~GangVideoCapturer() {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	rtc::CritScope cs(&crit_);
	stop();
	gang_ = NULL;
	delete[] static_cast<char*>(captured_frame_.data);
	SPDLOG_TRACE(console, "{} {}", __FUNCTION__, "ok")
}

GangVideoCapturer* GangVideoCapturer::Create(shared_ptr<GangDecoder> gang) {
	if (!gang.get()) {
		return NULL;
	}
	std::unique_ptr<GangVideoCapturer> capturer(new GangVideoCapturer(gang));
	if (!capturer.get()) {
		SPDLOG_TRACE(console, "{} {}", __FUNCTION__, "error")
		return NULL;
	}
	capturer->Initialize();
	return capturer.release();
}

void GangVideoCapturer::Initialize() {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	int width;
	int height;
	int fps;
	gang_->GetVideoInfo(&width, &height, &fps);

	captured_frame_.fourcc = cricket::FOURCC_I420;
	captured_frame_.pixel_height = 1;
	captured_frame_.pixel_width = 1;
	captured_frame_.width = width;
	captured_frame_.height = height;
	captured_frame_.data_size = static_cast<uint32>(cricket::VideoFrame::SizeOf(width, height));
	captured_frame_.data = new char[captured_frame_.data_size];

// Enumerate the supported formats. We have only one supported format. We set
// the frame interval to kMinimumInterval here. In Start(), if the capture
// format's interval is greater than kMinimumInterval, we use the interval;
// otherwise, we use the timestamp in the file to control the interval.
// TODO change supper fps to that from stream
	cricket::VideoFormat format(
			width,
			height,
			cricket::VideoFormat::FpsToInterval(fps),
			cricket::FOURCC_I420);
	std::vector<VideoFormat> supported;
	supported.push_back(format);
	SetSupportedFormats(supported);

// TODO(wuwang): Design an E2E integration test for video adaptation,
// then remove the below call to disable the video adapter.
	set_enable_video_adapter(false);
}

CaptureState GangVideoCapturer::Start(const VideoFormat& capture_format) {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	rtc::CritScope cs(&crit_);
	if (capture_) {
		return cricket::CS_FAILED;
	}
	SetCaptureFormat(&capture_format);
	start_time_ns_ = static_cast<int64>(rtc::TimeNanos());

	if (gang_->SetVideoFrameObserver(this, static_cast<uint8_t*>(captured_frame_.data))) {
		SPDLOG_TRACE(console, "{}: {}", __FUNCTION__, "ok")
		capture_ = true;
		return cricket::CS_RUNNING;
	}
	SPDLOG_TRACE(console, "{}: {}", __FUNCTION__, "failed")
	return cricket::CS_FAILED;
}

void GangVideoCapturer::stop() {
	if (capture_) {
		capture_ = false;
		if (IsRunning()) {
			SetCaptureFormat(NULL);
			gang_->SetVideoFrameObserver(NULL, NULL);
		}
	}
}

void GangVideoCapturer::Stop() {
	SPDLOG_TRACE(console, "{}", __FUNCTION__)
	rtc::CritScope cs(&crit_);
	stop();
	SPDLOG_TRACE(console, "{} {}", __FUNCTION__, "ok")
}

bool GangVideoCapturer::IsRunning() {
	return gang_.get() && gang_->IsRunning();
}

void GangVideoCapturer::OnGangFrame() {
	captured_frame_.time_stamp = static_cast<int64>(rtc::TimeNanos());
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
