#ifndef GANGSOURCES_H_
#define GANGSOURCES_H_
#pragma once

#include <memory>
#include "talk/app/webrtc/videosourceinterface.h"
#include "talk/app/webrtc/peerconnectioninterface.h"
#include "gangvideocapturer.h"

namespace gang {

class GangSources {
public:
	GangSources();
	~GangSources() {
	}
	std::shared_ptr<webrtc::VideoSourceInterface> GetVideo(
			const std::string& url);

	void RegistryVideo(
			webrtc::PeerConnectionFactoryInterface* peer_connection_factory,
			const std::string url);

	void RegistryDecoder(
			webrtc::PeerConnectionFactoryInterface* peer_connection_factory,
			const std::string url);
private:
	std::map<std::string, std::shared_ptr<webrtc::VideoSourceInterface> > videos_;
	GangVideoCapturerFactory video_capturer_factory_;
};

}  // namespace gang

#endif // GANGSOURCES_H_
