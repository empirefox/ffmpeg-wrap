#include <iostream>
#include <memory>

#include "../gang_init_deps.h"
#include "../gang_decoder.h"
#include "spdlog/spdlog.h"

using namespace std;
using namespace gang;
static int run(char *i_filename, char *o_filename) {
	int width = 0;
	int heith = 0;
	int fps = 0;
	bool rec = true;
//	string url = "rtsp://218.204.223.237:554/live/1/0547424F573B085C/gsfp90ef4k0a6iap.sdp";
	unique_ptr<GangDecoder> dec(new GangDecoder(i_filename, o_filename, rec));
	printf("unique_ptr<GangDecoder>\n");
	if (!dec.get()) {
		printf("GangDecoder instance error\n");
		return 1;
	}
	if (!dec->Init()) {
		printf("decoder init error\n");
		return 1;
	}

	dec->GetVideoInfo(&width, &heith, &fps);
	printf("best wdth %d \n", width);
	printf("best height %d \n", heith);
	printf("best fps %d \n", fps);

	dec->Start();
	cout << "Press Enter to Stop" << endl;
	cin.get();

	dec->Stop();

	cout << "Press Enter to Quit" << endl;
	cin.get();

	return 0;
}

int main(int argc, char **argv) {
	int ret;
	// MUST setup here!
	spdlog::set_level(spdlog::level::trace);
	InitializeGangDecoderGlobel();

	ret = run(argv[1], argv[2]);
	CleanupGangDecoderGlobel();

	cout << "Quit!" << endl;
	return ret;
}
