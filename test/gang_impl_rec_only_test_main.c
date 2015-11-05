#include <libavutil/log.h>

#include "macrologger.h"
#include "../gang_decoder_impl.h"

static int run(char *i_filename, char *o_filename) {
	int ret;

	gang_decoder *dec = new_gang_decoder(i_filename, o_filename, 1);
	if (!dec) {
		LOG_DEBUG("new_gang_decoder failed");
		return 1;
	}
	ret = open_gang_decoder(dec);
	if (ret) {
		LOG_DEBUG("open_gang_decoder failed");
		goto end;
	}

	while (1) {
		ret = gang_decode_next_frame(dec, NULL, 0);
		if (ret < 0) {
			break;
		}
	}
	ret = flush_gang_rec_encoder(dec);

	end: close_gang_decoder(dec);
	free_gang_decoder(dec);
	return ret ? 1 : 0;
}

int main(int argc, char **argv) {
	if (argc != 3) {
		av_log(NULL, AV_LOG_ERROR, "Usage: %s <input file> <output file>\n", argv[0]);
		return 1;
	}
	initialize_gang_decoder_globel();
	return run(argv[1], argv[2]);
}
