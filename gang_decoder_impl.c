#include <stdlib.h>
#include "gang_decoder_impl.h"

static int get_format_from_sample_fmt(
		const char **fmt,
		enum AVSampleFormat sample_fmt) {
	int i;
	struct sample_fmt_entry {
		enum AVSampleFormat sample_fmt;
		const char *fmt_be, *fmt_le;
	} sample_fmt_entries[] = {
			{
					AV_SAMPLE_FMT_U8,
					"u8",
					"u8" },
			{
					AV_SAMPLE_FMT_S16,
					"s16be",
					"s16le" },
			{
					AV_SAMPLE_FMT_S32,
					"s32be",
					"s32le" },
			{
					AV_SAMPLE_FMT_FLT,
					"f32be",
					"f32le" },
			{
					AV_SAMPLE_FMT_DBL,
					"f64be",
					"f64le" }, };
	*fmt = NULL;

	for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++) {
		struct sample_fmt_entry *entry = &sample_fmt_entries[i];
		if (sample_fmt == entry->sample_fmt) {
			*fmt = AV_NE(entry->fmt_be, entry->fmt_le);
			return 0;
		}
	}

	fprintf(
			stderr,
			"sample format %s is not supported as output format\n",
			av_get_sample_fmt_name(sample_fmt));
	return -1;
}

int decode_register_init = 0;
static int decode_init() {
	avcodec_register_all();
	av_register_all();
	avformat_network_init();
	decode_register_init = 1;
	return 1;
}

// create gang_decode with given url
struct gang_decoder* new_gang_decoder(const char* url) {
	struct gang_decoder * decoder = (struct gang_decoder*) malloc(
			sizeof(struct gang_decoder));
	decoder->url = url;
	decoder->video_stream_index = -1;
	decoder->audio_stream_index = -1;
	decoder->is_audio_planar_ = 0;

	decoder->i_fmt_ctx = NULL;
	decoder->video_dec_ctx = NULL;
	decoder->audio_dec_ctx = NULL;
	decoder->video_dst_data[0] = NULL;

	return decoder;
}

int init_gang_decoder(struct gang_decoder* decoder) {
	if (decode_register_init == 0) {
		decode_init();
	}

	AVFormatContext *i_fmt_ctx = NULL;
	unsigned video_stream_index = -1;
	unsigned audio_stream_index = -1;

	AVCodecContext* video_dec_ctx = NULL;
	AVCodec* video_dec = NULL;
	AVCodecContext* audio_dec_ctx = NULL;
	AVCodec* audio_dec = NULL;

	i_fmt_ctx = avformat_alloc_context();

	if (avformat_open_input(&i_fmt_ctx, decoder->url, NULL, NULL) != 0) {
		fprintf(stderr, "could not open input file\n");
		return -1;
	}

	if (avformat_find_stream_info(i_fmt_ctx, NULL) < 0) {
		fprintf(stderr, "could not find stream info\n");
		avformat_close_input(&i_fmt_ctx);
		return -1;
	}

	/* find stream index */
	for (unsigned i = 0; i < i_fmt_ctx->nb_streams; i++) {
		if (i_fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {

			printf("video_stream_index: %d\n", i);
			video_stream_index = i;
			//break;
		} else if (i_fmt_ctx->streams[i]->codec->codec_type
				== AVMEDIA_TYPE_AUDIO) {
			printf("audio_stream_index: %d\n", i);
			audio_stream_index = i;
		}
	}
	if (video_stream_index != -1) {
		video_dec_ctx = i_fmt_ctx->streams[video_stream_index]->codec;
		video_dec = avcodec_find_decoder(video_dec_ctx->codec_id);
		if (avcodec_open2(video_dec_ctx, video_dec, NULL) < 0) {
			avformat_close_input(&i_fmt_ctx);
			return 0;
		}
		decoder->width = video_dec_ctx->width;
		decoder->height = video_dec_ctx->height;
		decoder->fps = 24;
		decoder->pix_fmt = video_dec_ctx->pix_fmt;
		fprintf(
				stderr,
				"init format: %s\n",
				av_get_pix_fmt_name(video_dec_ctx->pix_fmt));
	}

	if (audio_stream_index != -1) {
		audio_dec_ctx = i_fmt_ctx->streams[audio_stream_index]->codec;
		enum AVSampleFormat sfmt = audio_dec_ctx->sample_fmt;
		const char *fmt;

		if (av_sample_fmt_is_planar(sfmt)) {
			const char *packed = av_get_sample_fmt_name(sfmt);
			printf(
					"Warning: the sample format the decoder produced is planar "
							"(%s). This example will output the first channel only.\n",
					packed ? packed : "?");
			sfmt = av_get_packed_sample_fmt(sfmt);
			decoder->is_audio_planar_ = 1;
		}

		if ((get_format_from_sample_fmt(&fmt, sfmt)) < 0)
			printf("get_format_from_sample_fmt < 0");

		decoder->channels = audio_dec_ctx->channels;
//		decoder->channels = 1;
		decoder->sample_rate = audio_dec_ctx->sample_rate;
		decoder->bytesPerSample = av_get_bytes_per_sample(
				audio_dec_ctx->sample_fmt);
		printf("sfmt: %s\n", av_get_sample_fmt_name(sfmt));
		printf("fmt: %s\n", fmt);
		printf("channels: %d\n", decoder->channels);
		printf("sample_rate: %d\n", decoder->sample_rate);
	}

	avformat_close_input(&i_fmt_ctx);
	return 1;
}
// prepare AVCodecContext... and store to struct
int start_gang_decode(struct gang_decoder* decoder_) {
	int size = 0;

	if (decode_register_init == 0) {
		decode_init();
	}

	AVCodec* video_dec = NULL;

	AVCodec* audio_dec = NULL;

	decoder_->i_fmt_ctx = avformat_alloc_context();

	if (avformat_open_input(&decoder_->i_fmt_ctx, decoder_->url, NULL, NULL)
			!= 0) {
		fprintf(stderr, "start_gang_decode: could not open input file\n");
		return -1;
	}

	if (avformat_find_stream_info(decoder_->i_fmt_ctx, NULL) < 0) {
		fprintf(stderr, "could not find stream info\n");
		return -1;
	}

	/* find stradms index */
	for (unsigned i = 0; i < decoder_->i_fmt_ctx->nb_streams; i++) {
		if (decoder_->i_fmt_ctx->streams[i]->codec->codec_type
				== AVMEDIA_TYPE_VIDEO) {

			decoder_->video_stream_index = i;
			//break;
		} else if (decoder_->i_fmt_ctx->streams[i]->codec->codec_type
				== AVMEDIA_TYPE_AUDIO) {
			decoder_->audio_stream_index = i;
		}
	}
	if (decoder_->video_stream_index != -1) {
		decoder_->video_dec_ctx =
				decoder_->i_fmt_ctx->streams[decoder_->video_stream_index]->codec;
		video_dec = avcodec_find_decoder(decoder_->video_dec_ctx->codec_id);
		if (avcodec_open2(decoder_->video_dec_ctx, video_dec, NULL) < 0) {
			return 0;
		}
	}
	if (decoder_->audio_stream_index != -1) {
		decoder_->audio_dec_ctx =
				decoder_->i_fmt_ctx->streams[decoder_->audio_stream_index]->codec;
		audio_dec = avcodec_find_decoder(decoder_->audio_dec_ctx->codec_id);
		if (avcodec_open2(decoder_->audio_dec_ctx, audio_dec, NULL) < 0) {
			return 0;
		}
	}

	av_init_packet(&decoder_->i_pkt);

	/* allocate image where the decoded image will be put */
	size = av_image_alloc(
			decoder_->video_dst_data,
			decoder_->video_dst_linesize,
			decoder_->width,
			decoder_->height,
			decoder_->pix_fmt,
			1);
	if (size < 0) {
		fprintf(stderr, "Could not allocate raw video buffer\n");
		return 0;
	}
	decoder_->video_dst_bufsize = size;
	return 1;
}

int gang_decode_next_frame(struct gang_decoder* decoder, void **data, int *size) {

	if (av_read_frame(decoder->i_fmt_ctx, &decoder->i_pkt) < 0) {
		fprintf(stderr, "av_read_frame error!\n");
		return 0;
	}
	AVFrame *pFrame = av_frame_alloc();
	int got_picture = 0, ret = 0;

	//video
	if (decoder->i_pkt.stream_index == decoder->video_stream_index) {

		avcodec_decode_video2(
				decoder->video_dec_ctx,
				pFrame,
				&got_picture,
				&decoder->i_pkt);
		if (got_picture) {

			av_image_copy(
					decoder->video_dst_data,
					decoder->video_dst_linesize,
					(const uint8_t **) (pFrame->data),
					pFrame->linesize,
					decoder->pix_fmt,
					decoder->width,
					decoder->height);

			*data = (uint8_t*) malloc(decoder->video_dst_bufsize);
			memcpy(
					*data,
					decoder->video_dst_data[0],
					decoder->video_dst_bufsize);
			*size = decoder->video_dst_bufsize;
			//*pts = decoder->i_pkt.pts;
			ret = 1;

		} else {
			fprintf(stderr, "decode video frame error!\n");
			goto end;
		}

	} else if (decoder->i_pkt.stream_index == decoder->audio_stream_index) {
		/* decode audio frame */
		ret = avcodec_decode_audio4(
				decoder->audio_dec_ctx,
				pFrame,
				&got_picture,
				&decoder->i_pkt);
		if (ret < 0) {
			fprintf(
			stderr, "Error decoding audio frame (%s)\n", av_err2str(ret));
			return ret;
		}

		if (got_picture) {
			size_t unpadded_linesize = pFrame->nb_samples
					* decoder->bytesPerSample * decoder->channels;
			if (unpadded_linesize < 1) {
				fprintf(stderr, "decode audio frame error! length < 1\n");
				goto end;
			}

			/* Write the raw audio data samples of the first plane. This works
			 * fine for packed formats (e.g. AV_SAMPLE_FMT_S16). However,
			 * most audio decoders output planar audio, which uses a separate
			 * plane of audio samples for each channel (e.g. AV_SAMPLE_FMT_S16P).
			 * In other words, this code will write only the first audio channel
			 * in these cases.
			 * You should use libswresample or libavfilter to convert the frame
			 * to packed data. */

			uint8_t* tmp = (uint8_t*) malloc(unpadded_linesize);
			if (decoder->is_audio_planar_) {
				int i, ch;
				uint8_t* t = tmp;
				for (i = 0; i < pFrame->nb_samples; i++) {
					for (ch = 0; ch < decoder->channels; ++ch) {
						memcpy(
								t,
								pFrame->data[ch] + decoder->bytesPerSample * i,
								decoder->bytesPerSample);
						t += decoder->bytesPerSample;
					}
				}
			} else {
				memcpy(tmp, pFrame->extended_data[0], unpadded_linesize);
			}

			*data = tmp;
			*size = pFrame->nb_samples;
			//*pts = decoder_->i_pkt.pts;
			ret = 2;

		} else {
			fprintf(stderr, "decode audio frame error!\n");
			goto end;
		}
	}
	end: av_free_packet(&decoder->i_pkt);
	av_frame_free(&pFrame);
	return ret;

}

// disconnect from remote stream and free AVCodecContext...
void stop_gang_decode(struct gang_decoder* decoder_) {
	avformat_close_input(&decoder_->i_fmt_ctx);
	av_free(decoder_->video_dst_data[0]);
}

// free gang_decoder
void free_gang_decode(struct gang_decoder* decoder) {
	free(decoder);
}

