#include  "gang_decoder.hh"
#include  "gang_decoder.h"

namespace gang {

GangDecoder::GangDecoder(const char* url) :
		decoder_(::new_gang_decoder(url)) {
}

GangDecoder::~GangDecoder() {
	::free_gang_decode(decoder_);
}

bool GangDecoder::Init() {
	return ::init_gang_decoder(decoder_);
}

bool GangDecoder::Start() {
	return ::start_gang_decode(decoder_);
}

void GangDecoder::Stop() {
	::stop_gang_decode(decoder_);
}

GangFrame GangDecoder::NextFrame() {
	gang_frame frame_ = ::gang_decode_next_frame(decoder_);
	GangFrame frame;
	frame.data = reinterpret_cast<uint8*>(frame_.data);
	frame.size = static_cast<uint32>(frame_.size);
	frame.is_video = frame_.is_video == 1;
	return frame;
}

}  // namespace gang
