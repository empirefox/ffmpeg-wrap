#include "gangsources.h"
#include "gangvideocapturerfactory.h"

namespace gang {

GangSources::GangSources() {
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
	cricket::VideoCapturer* capturer = video_capturer_factory_.Create(
			GangVideoCapturer::CreateGangVideoCapturerDevice(url));
	if (capturer != NULL) {
		videos_.insert(
				std::make_pair(url,
						std::shared_ptr<webrtc::VideoSourceInterface>(
								peer_connection_factory->CreateVideoSource(
										capturer,
										NULL))));
	}
}

void GangSources::RegistryDecoder(
		webrtc::PeerConnectionFactoryInterface* peer_connection_factory,
		const std::string url) {
	//1 create decoder

	//2 registry video capturer

	//2.1 create video capturer

	//2.2 mount to decoder

	//2.3 create source and insert to sources

	//3 registry audio module

	//3.1 create audio capturer

	//3.2 mount to decoder

	//3.3 create source and insert to sources
	GangVideoCapturer* capturer = video_capturer_factory_.GangCreate(
			GangVideoCapturer::CreateGangVideoCapturerDevice(url));
	if (capturer != NULL) {
		videos_.insert(
				std::make_pair(url,
						std::shared_ptr<webrtc::VideoSourceInterface>(
								peer_connection_factory->CreateVideoSource(
										capturer,
										NULL))));
	}
}

// Detail: http://stackoverflow.com/a/13280852/2778814
// Technique covered in many texts.
static int count = 0; // This gets implicitly initialized to 0 by the executable load into memory.
static struct singleton_sources_class {
	static std::shared_ptr<GangSources> ptr;

	singleton_sources_class() {
		if (count++ == 0) {
			ptr = std::make_shared<GangSources>(new GangSources()); //initialization
		}
	}
} singleton_sources;

std::shared_ptr<GangSources> singleton_sources_class::ptr =
		count == 0 ?
				std::make_shared<GangSources>(new GangSources()) :
				move(singleton_sources_class::ptr);

}  // namespace gang

