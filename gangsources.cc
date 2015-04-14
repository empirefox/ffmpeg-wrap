#include "gangsources.h"
#include "gangvideocapturerfactory.h"

namespace gang {

GangSources::GangSources() :
		video_capturer_factory_(
				std::unique_ptr<GangVideoCapturerFactory>(
						new GangVideoCapturerFactory())) {
}

std::shared_ptr<webrtc::VideoSourceInterface> GangSources::GetVideo(
		const std::string& url) {
	std::map<std::string, std::shared_ptr<webrtc::VideoSourceInterface> >::iterator iter =
			videos_.find(url);
	if (iter != videos_.end()) {
		return *iter;
	}
	return NULL;
}

void GangSources::RegistryVideo(
		webrtc::PeerConnectionFactoryInterface* peer_connection_factory,
		const std::string url) {
	videos_.insert(
			std::make_pair(url,
					std::shared_ptr<webrtc::VideoSourceInterface>(
							peer_connection_factory->CreateVideoSource(
									video_capturer_factory_->Create(
											GangVideoCapturer::CreateGangVideoCapturerDevice(
													url)),
									NULL))));
}

// Detail: http://stackoverflow.com/a/13280852/2778814
// Technique covered in many texts.
static int count = 0; // This gets implicitly initialized to 0 by the executable load into memory.
static struct myclass {
	static std::shared_ptr<GangSources> ptr;

	myclass() {
		if (count++ == 0) {
			ptr = std::make_shared<GangSources>(new GangSources()); //initialization
		}
	}
} singleton_sources;

std::shared_ptr<GangSources> myclass::ptr =
		count == 0 ?
				std::make_shared<GangSources>(new GangSources()) :
				move(myclass::ptr);

}  // namespace gang

