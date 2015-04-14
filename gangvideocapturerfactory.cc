#include "gangvideocapturerfactory.h"
#include "gangvideocapturer.h"
#include "webrtc/base/scoped_ptr.h"

using namespace cricket;
namespace gang {

VideoCapturer* GangVideoCapturerFactory::Create(const Device& device) {
	rtc::scoped_ptr<GangVideoCapturer> capturer(new GangVideoCapturer());
	if (!capturer->Init(device)) {
		return NULL;
	}
	return capturer.release();
}

}  // namespace gang

