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

std::shared_ptr<GangDecoder> GangSources::GetDecoder(
		const std::string& url) {
	std::map<std::string, std::shared_ptr<GangDecoder> >::iterator iter =
			decoders_.find(url);
	if (iter != decoders_.end()) {
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

}  // namespace gang

