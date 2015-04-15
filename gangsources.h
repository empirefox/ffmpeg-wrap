#ifndef GANGSOURCES_H_
#define GANGSOURCES_H_
#pragma once

#include <memory>
#include "talk/app/webrtc/videosourceinterface.h"
#include "talk/app/webrtc/peerconnectioninterface.h"
#include "gangvideocapturer.h"

namespace gang {

// Connect sources to GangDecoder
// Provide video/audio source by url
class GangSources {
public:
	GangSources();
	~GangSources() {
	}
	std::shared_ptr<webrtc::VideoSourceInterface> GetVideo(
			const std::string& url);

	// TODO not yet implemented by RegistryDecoder
	std::shared_ptr<webrtc::AudioSourceInterface> GetAudio(
			const std::string& url);

	void RegistryDecoder(
			webrtc::PeerConnectionFactoryInterface* peer_connection_factory,
			const std::string url);
private:
	std::map<std::string, std::shared_ptr<webrtc::VideoSourceInterface> > videos_;
	std::map<std::string, std::shared_ptr<webrtc::AudioSourceInterface> > audios_;
	std::map<std::string, std::shared_ptr<GangDecoder> > decoders_;
	GangVideoCapturerFactory video_capturer_factory_;
};

}  // namespace gang

#endif // GANGSOURCES_H_
