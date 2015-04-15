#include "talk/media/base/videocapturerfactory.h"
#include "gangvideocapturer.h"

namespace gang {
class GangVideoCapturerFactory: public cricket::VideoDeviceCapturerFactory {
public:
	GangVideoCapturerFactory() {
	}
	virtual ~GangVideoCapturerFactory() {
	}

	virtual cricket::VideoCapturer* Create(const cricket::Device& device);

	virtual GangVideoCapturer* GangCreate(const cricket::Device& device);
};

}  // namespace gang
