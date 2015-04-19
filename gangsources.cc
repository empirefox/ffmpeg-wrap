#include "gangsources.h"

namespace gang {

GangSources::GangSources() {
}

rtc::scoped_refptr<webrtc::VideoSourceInterface> GangSources::GetVideo(
		const std::string& url) {
	std::map<std::string, rtc::scoped_refptr<webrtc::VideoSourceInterface> >::iterator iter =
			videos_.find(url);
	if (iter != videos_.end()) {
		return iter->second;
	}
	return NULL;
}

rtc::scoped_refptr<webrtc::AudioSourceInterface> GangSources::GetAudio(
		const std::string& url) {
	std::map<std::string, rtc::scoped_refptr<webrtc::AudioSourceInterface> >::iterator iter =
			audios_.find(url);
	if (iter != audios_.end()) {
		return iter->second;
	}
	return NULL;
}

void GangSources::RegistryDecoder(
		webrtc::PeerConnectionFactoryInterface* peer_connection_factory,
		const std::string url) {
	//1 create decoder
	std::shared_ptr<GangDecoder> decoder(new GangDecoder(url));
	if (!decoder->Init()) {
		return;
	}
	decoders_.insert(std::make_pair(url, decoder));

	//2 registry video capturer

	//2.1 create video capturer, capturer will be managed by peer connection
	GangVideoCapturer* capturer = new GangVideoCapturer(url);
	capturer->Init(decoder.get());

	//2.2 mount to decoder
	decoder->SetVideoFrameObserver(capturer);

	//2.3 create source and insert to sources
	if (capturer != NULL) {
		videos_.insert(
				std::make_pair(url,
						peer_connection_factory->CreateVideoSource(capturer,
						NULL)));
	}

	//3 registry audio module if audio exists

	//3.1 create audio capturer

	//3.2 mount to decoder

	//3.3 create source and insert to sources
}

// Detail: http://stackoverflow.com/a/13280852/2778814
// Technique covered in many texts.
static int count = 0; // This gets implicitly initialized to 0 by the executable load into memory.
static struct singleton_sources_class {
	static std::shared_ptr<GangSources> ptr;

	singleton_sources_class() {
		if (count++ == 0) {
			ptr = std::make_shared<GangSources>(); //initialization
		}
	}
} singleton_sources;

std::shared_ptr<GangSources> singleton_sources_class::ptr =
		count == 0 ?
				std::make_shared<GangSources>() :
				move(singleton_sources_class::ptr);

}  // namespace gang

