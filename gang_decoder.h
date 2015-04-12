#ifndef GANG_GANG_DECODER_HH
#define GANG_GANG_DECODER_HH

#include "webrtc/base/basictypes.h"
#include "webrtc/base/constructormagic.h"
#include "gang_decoder.h"
#include "gang_decoder_impl.h"

namespace gang {

struct GangFrame {
	uint8* data;
	uint32 size;
	bool is_video;
};

class GangDecoder {
public:
	explicit GangDecoder(const char* url);
	~GangDecoder();

	bool Init();
	bool Start();
	void Stop();

	int GetBestWidth();
	int GetBestHeight();
	int GetBestFps();

	GangFrame NextFrame();
private:
	gang_decoder* decoder_;

	DISALLOW_COPY_AND_ASSIGN(GangDecoder);
};

}  // namespace gang

#endif // GANG_GANG_DECODER_HH
