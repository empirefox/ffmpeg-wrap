/*
 * gang_init_deps.cc
 *
 *  Created on: May 2, 2015
 *      Author: savage
 */

#include "gang_init_deps.h"

#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"

#include "gang_decoder_impl.h"
#include "spdlog_console.h"

namespace gang {

void InitializeGangDecoderGlobel() {
	::initialize_gang_decoder_globel();
	WebRtcSpl_Init();
	InitSpdlogConsole();
}

void CleanupGangDecoderGlobel() {
	CleanupSpdlog();
}

} // namespace gang

