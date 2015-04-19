#ifndef GANGSOURCES_H_
#define GANGSOURCES_H_
#pragma once

#include <memory>
#include "talk/app/webrtc/videosourceinterface.h"
#include "talk/app/webrtc/peerconnectioninterface.h"
#include "gangvideocapturerfactory.h"
#include "gangvideocapturer.h"
#include "gang_decoder.h"

namespace gang {

// Connect sources to GangDecoder
// Provide video/audio source by url
class GangSources {
public:
	GangSources();
	~GangSources() {
	}
	rtc::scoped_refptr<webrtc::VideoSourceInterface> GetVideo(
			const std::string& url);

	// TODO not yet implemented by RegistryDecoder
	rtc::scoped_refptr<webrtc::AudioSourceInterface> GetAudio(
			const std::string& url);

	void RegistryDecoder(
			webrtc::PeerConnectionFactoryInterface* peer_connection_factory,
			const std::string url);
private:
	std::map<std::string, rtc::scoped_refptr<webrtc::VideoSourceInterface> > videos_;
	std::map<std::string, rtc::scoped_refptr<webrtc::AudioSourceInterface> > audios_;
	std::map<std::string, std::shared_ptr<GangDecoder> > decoders_;
};

struct singleton_sources_class {
	static std::shared_ptr<GangSources> ptr;
	singleton_sources_class();
};

}  // namespace gang

#endif // GANGSOURCES_H_
