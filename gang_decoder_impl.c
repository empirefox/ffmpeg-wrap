#include <libavutil/imgutils.h>

#include "gang_decoder_impl.h"
#include "ffmpeg_format.h"

// create gang_decode with given url
struct gang_decoder *new_gang_decoder(const char *url) {
	struct gang_decoder *decoder = (struct gang_decoder*) malloc(
			sizeof(struct gang_decoder));
	decoder->url = (char*) url;
	decoder->no_video = 1;
	decoder->no_audio = 1;

	decoder->width = 0;
	decoder->height = 0;
	decoder->fps = 0;
	decoder->pix_fmt = AV_PIX_FMT_NONE;

	decoder->channels = 0;
	decoder->sample_rate = 0;
	decoder->bytes_per_sample = 2; // always output s16
	decoder->s16_status = AV_SAMPLE_FMT_NONE;

	decoder->video_dst_data[0] = NULL;
	decoder->video_dst_bufsize = 0;

	decoder->audio_dst_data = NULL;
	decoder->audio_dst_max_nb_samples = 0;
	decoder->audio_dst_linesize = 0;
	decoder->audio_dst_nb_samples = 0;

	decoder->i_fmt_ctx = NULL;
	decoder->swr_ctx = NULL;

	decoder->video_stream = NULL;
	decoder->audio_stream = NULL;

	decoder->i_frame = NULL;

	return decoder;
}

void free_gang_decoder(struct gang_decoder *decoder) {
	free(decoder);
}

void init_gang_video_info(struct gang_decoder *decoder) {
	decoder->pix_fmt = decoder->video_stream->codec->pix_fmt;
	decoder->width = decoder->video_stream->codec->width;
	decoder->height = decoder->video_stream->codec->height;
	AVRational rate = decoder->video_stream->r_frame_rate;
	if (rate.den) {
		decoder->fps = rate.num / rate.den;
	} else {
		// TODO maybe need calculate it.
		decoder->fps = 10000;
	}
	fprintf(
			stderr,
			"init_gang_video_info pix_fmt:%s, fps:%d\n",
			av_get_pix_fmt_name(decoder->pix_fmt),
			decoder->fps);
}

void init_gang_audio_info(struct gang_decoder *decoder) {
	decoder->channels = decoder->audio_stream->codec->channels;
	decoder->sample_rate = decoder->audio_stream->codec->sample_rate;
	fprintf(
			stderr,
			"init_gang_audio_info no resample, channels:%d, sample_rate:%d\n",
			decoder->channels,
			decoder->sample_rate);
}

// return error
int init_gang_video_decode_buffer(struct gang_decoder *decoder) {
	/* allocate image where the decoded image will be put */
	int size = av_image_alloc(
			decoder->video_dst_data,
			decoder->video_dst_linesize,
			decoder->width,
			decoder->height,
			decoder->pix_fmt,
			1);
	if (size < 0) {
		fprintf(stderr, "Could not allocate raw video buffer\n");
		return -1;
	}
	decoder->video_dst_bufsize = size;
	return 0;
}

// return error
int open_gang_decoder(struct gang_decoder *decoder) {
	int error;
	av_register_all();
	avformat_network_init();

	error = open_input_file(
			decoder->url,
			&decoder->i_fmt_ctx,
			&decoder->video_stream,
			&decoder->audio_stream);
	if (error) {
		return error;
	}

	error = init_audio_resampler(
			decoder->audio_stream->codec,
			&decoder->swr_ctx,
			&decoder->channels,
			&decoder->sample_rate,
			&decoder->s16_status);
	if (error) {
		return error;
	}

	init_gang_video_info(decoder);
	if (!decoder->swr_ctx) {
		// Do not need resample, so init origin info here.
		init_gang_audio_info(decoder);
	} else {
		fprintf(
				stderr,
				"init_gang_audio_info WITH resample, channels:%d, sample_rate:%d\n",
				decoder->channels,
				decoder->sample_rate);
		fprintf(
				stderr,
				"origin sample format:%s\n",
				av_get_sample_fmt_name(
						decoder->audio_stream->codec->sample_fmt));
	}

	av_init_packet(&decoder->i_pkt);

	// must be called after init_gang_video_info
	error = init_gang_video_decode_buffer(decoder);
	if (error) {
		return error;
	}

	error = init_input_frame(&decoder->i_frame);
	if (error) {
		return error;
	}

	decoder->no_video = !decoder->video_stream;
	decoder->no_audio = !decoder->audio_stream;
	fprintf(stderr, "open_gang_decoder ok\n");

	return 0;
}

void close_gang_decoder(struct gang_decoder *decoder) {
	if (decoder->i_fmt_ctx)
		avformat_close_input(&decoder->i_fmt_ctx);
	// free av_image_alloc
	if (decoder->video_dst_data[0])
		av_free(decoder->video_dst_data[0]);
	decoder->video_stream = NULL;
	decoder->audio_stream = NULL;
	// free SwrContext
	if (decoder->swr_ctx)
		swr_free(&decoder->swr_ctx);
	if (decoder->audio_dst_data)
		av_freep(&decoder->audio_dst_data[0]);
	av_freep(&decoder->audio_dst_data);
	if (decoder->i_frame)
		av_frame_free(&decoder->i_frame);
	fprintf(stderr, "close_gang_decoder ok\n");
}

// return error
int gang_decode_next_video(struct gang_decoder* decoder, void **data, int *size) {

	int error, got_picture = 0;
	error = avcodec_decode_video2(
			decoder->video_stream->codec,
			decoder->i_frame,
			&got_picture,
			&decoder->i_pkt);

	if (error < 0) {
		fprintf(stderr, "Error decoding video frame (%s)\n", av_err2str(error));
		return error;
	}

	if (!got_picture) {
		fprintf(stderr, "Cannot decode video frame!\n");
		return -1;
	}

	av_image_copy(
			decoder->video_dst_data,
			decoder->video_dst_linesize,
			(const uint8_t **) (decoder->i_frame->data),
			decoder->i_frame->linesize,
			decoder->pix_fmt,
			decoder->width,
			decoder->height);

	*data = (uint8_t*) malloc(decoder->video_dst_bufsize);
	memcpy(*data, decoder->video_dst_data[0], decoder->video_dst_bufsize);
	*size = decoder->video_dst_bufsize;

	return 0;
}

int prepare_resample_buffer(struct gang_decoder* decoder) {
	int ret; // error

	decoder->audio_dst_nb_samples = av_rescale_rnd(
			decoder->i_frame->nb_samples,
			decoder->sample_rate,
			decoder->audio_stream->codec->sample_rate,
			AV_ROUND_UP);

	if (decoder->audio_dst_nb_samples > decoder->audio_dst_max_nb_samples) {
		if (decoder->audio_dst_data) {
			av_freep(&decoder->audio_dst_data[0]);
			ret = av_samples_alloc(
					decoder->audio_dst_data,
					&decoder->audio_dst_linesize,
					decoder->channels,
					decoder->audio_dst_nb_samples,
					AV_SAMPLE_FMT_S16,
					1);
		} else {
			ret = av_samples_alloc_array_and_samples(
					&decoder->audio_dst_data,
					&decoder->audio_dst_linesize,
					decoder->channels,
					decoder->audio_dst_nb_samples,
					AV_SAMPLE_FMT_S16,
					0);
		}
		if (ret < 0) {
			fprintf(
					stderr,
					"Could not allocate destination samples (%s)\n",
					av_err2str(ret));
			return ret;
		}
		decoder->audio_dst_max_nb_samples = decoder->audio_dst_nb_samples;
	}
	return 0;
}

int resample_and_copy(
		struct gang_decoder* decoder,
		void **data,
		int *nb_samples) {
	int ret; // error

	if (!decoder->swr_ctx) {
		fprintf(stderr, "Swr_ctx not inited\n");
		return -1;
	}

	ret = prepare_resample_buffer(decoder);
	if (ret < 0) {
		return ret;
	}

	/* convert to destination format */
	ret = swr_convert(
			decoder->swr_ctx,
			decoder->audio_dst_data,
			decoder->audio_dst_nb_samples,
			(const uint8_t **) decoder->i_frame->extended_data,
			decoder->i_frame->nb_samples);
	if (ret < 0) {
		fprintf(stderr, "Error while converting (%s)\n", av_err2str(ret));
		return ret;
	}

	int dst_bufsize = av_samples_get_buffer_size(
			&decoder->audio_dst_linesize,
			decoder->channels,
			ret, // output nb_samples
			AV_SAMPLE_FMT_S16,
			1);
	if (dst_bufsize < 0) {
		fprintf(
		stderr, "Could not get sample buffer size (%s)\n", av_err2str(ret));
		return ret;
	}

	uint8_t* tmp = (uint8_t*) malloc(dst_bufsize);
	memcpy(tmp, decoder->audio_dst_data[0], dst_bufsize);

	*data = tmp;
	*nb_samples = decoder->audio_dst_nb_samples;
	return 0;
}

// return error
int gang_decode_next_audio(
		struct gang_decoder* decoder,
		void **data,
		int *nb_samples) {

	int error, got_picture = 0;
	error = avcodec_decode_audio4(
			decoder->audio_stream->codec,
			decoder->i_frame,
			&got_picture,
			&decoder->i_pkt);
	if (error < 0) {
		fprintf(
		stderr, "Error decoding audio frame (%s)\n", av_err2str(error));
		return error;
	}

	if (!got_picture) {
		fprintf(stderr, "Cannot decode audio frame!\n");
		return -1;
	}

	size_t output_linesize = decoder->i_frame->nb_samples
			* decoder->bytes_per_sample * decoder->channels;
	if (output_linesize < 1) {
		fprintf(stderr, "decode audio frame error! length < 1\n");
		return -1;
	}

	if (decoder->s16_status == AV_SAMPLE_FMT_NONE) {
		return resample_and_copy(decoder, data, nb_samples);
	}

	uint8_t* tmp = (uint8_t*) malloc(output_linesize);
	if (decoder->s16_status == AV_SAMPLE_FMT_S16P) {
		s16p_2_s16(tmp, decoder->i_frame, decoder->channels);
	} else {
		memcpy(tmp, decoder->i_frame->extended_data[0], output_linesize);
	}

	*data = tmp;
	*nb_samples = decoder->i_frame->nb_samples;
	return 0;
}

/**
 * return: -1->EOF, 0->error, 1->video, 2->audio
 */
int gang_decode_next_frame(struct gang_decoder* decoder, void **data, int *size) {

	if (av_read_frame(decoder->i_fmt_ctx, &decoder->i_pkt) < 0) {
		fprintf(stderr, "av_read_frame error!\n");
		// TODO AVERROR_EOF?
		return GANG_EOF;
	}

	if (decoder->video_stream
			&& decoder->video_stream->index == decoder->i_pkt.stream_index
			&& !gang_decode_next_video(decoder, data, size)) {
		av_free_packet(&decoder->i_pkt);
		return GANG_VIDEO_DATA;
	} else if (decoder->audio_stream
			&& decoder->audio_stream->index == decoder->i_pkt.stream_index
			&& !gang_decode_next_audio(decoder, data, size)) {
		av_free_packet(&decoder->i_pkt);
		return GANG_AUDIO_DATA;
	}
	return GANG_ERROR_DATA;
}
