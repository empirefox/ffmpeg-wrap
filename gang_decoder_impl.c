#include <stdlib.h>
#include "gang_decoder_impl.h"

int decode_register_init = 0;
static int decode_init(){
	avcodec_register_all();
	av_register_all();
	avformat_network_init();
	decode_register_init = 1;
	return 1;
}

// create gang_decode with given url
struct gang_decoder* new_gang_decoder(const char* url){
	struct gang_decoder * decoder = (struct gang_decoder*)malloc(sizeof(struct gang_decoder));
	decoder->url = url;
	//decoder->best_width = 0;
	//decoder->best_height = 0;
	//decoder->best_fps = 20;
	decoder->video_stream_index = -1;
	decoder->audio_stream_index = -1;

	decoder->i_fmt_ctx     = NULL;
	decoder->video_dec_ctx = NULL;
	decoder->audio_dec_ctx = NULL;

	return decoder;
}

int init_gang_decoder(struct gang_decoder* decoder, int* best_width, int* best_height, int* best_fps)
{
	if (decode_register_init == 0){
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

	if (avformat_open_input(&i_fmt_ctx, decoder->url, NULL, NULL) != 0)
	{
		fprintf(stderr, "could not open input file\n");
		return -1;
	}

	if (avformat_find_stream_info(i_fmt_ctx, NULL)<0)
	{
		fprintf(stderr, "could not find stream info\n");
		return -1;
	}

	/* find stream index */
	for (unsigned i = 0; i<i_fmt_ctx->nb_streams; i++)
	{
		if (i_fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{

			video_stream_index = i;
			//break;
		}
		else if (i_fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
			audio_stream_index = i;
		}
	}
	if (video_stream_index != -1){
		video_dec_ctx = i_fmt_ctx->streams[video_stream_index]->codec;
		video_dec = avcodec_find_decoder(video_dec_ctx->codec_id);
		if (avcodec_open2(video_dec_ctx, video_dec, NULL) < 0)
		{
			return 0;
		}
		*best_width = video_dec_ctx->width;
		*best_height = video_dec_ctx->height;
		*best_fps = 20;
	}

	avformat_close_input(&i_fmt_ctx);
	return 1;
}
// prepare AVCodecContext... and store to struct
int start_gang_decode(struct gang_decoder* decoder_){
	if (decode_register_init == 0){
		decode_init();
	}

	AVCodec* video_dec = NULL;
	
	AVCodec* audio_dec = NULL;

	decoder_->i_fmt_ctx = avformat_alloc_context();

	if (avformat_open_input(&decoder_->i_fmt_ctx, decoder_->url, NULL, NULL) != 0)
	{
		fprintf(stderr, "could not open input file\n");
		return -1;
	}

	if (avformat_find_stream_info(decoder_->i_fmt_ctx, NULL)<0)
	{
		fprintf(stderr, "could not find stream info\n");
		return -1;
	}

	/* find stradms index */
	for (unsigned i = 0; i<decoder_->i_fmt_ctx->nb_streams; i++)
	{
		if (decoder_->i_fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{

			decoder_->video_stream_index = i;
			//break;
		}
		else if (decoder_->i_fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
			decoder_->audio_stream_index = i;
		}
	}
	if (decoder_->video_stream_index != -1){
		decoder_->video_dec_ctx = decoder_->i_fmt_ctx->streams[decoder_->video_stream_index]->codec;
		video_dec = avcodec_find_decoder(decoder_->video_dec_ctx->codec_id);
		if (avcodec_open2(decoder_->video_dec_ctx, video_dec, NULL) < 0)
		{
			return 0;
		}
	}
	if (decoder_->audio_stream_index != -1){
		decoder_->audio_dec_ctx = decoder_->i_fmt_ctx->streams[decoder_->audio_stream_index]->codec;
		audio_dec = avcodec_find_decoder(decoder_->audio_dec_ctx->codec_id);
		if (avcodec_open2(decoder_->audio_dec_ctx, audio_dec, NULL) < 0)
		{
			return 0;
		}
	}

	av_init_packet(&decoder_->i_pkt);
	return 1;
}

int gang_decode_next_frame(struct gang_decoder* decoder, uint8_t **data, int *size){

	if (av_read_frame(decoder->i_fmt_ctx, &decoder->i_pkt) < 0){
		fprintf(stderr, "av_read_frame error!\n");
		return 0;
	}
	AVFrame *pFrame = av_frame_alloc();
	int got_picture = 0, ret = 0;
	
	//video
	if (decoder->i_pkt.stream_index == decoder->video_stream_index) {

		avcodec_decode_video2(decoder->video_dec_ctx, pFrame, &got_picture, &decoder->i_pkt);
		if (got_picture) {

			*data = malloc(decoder->video_dec_ctx->height * decoder->video_dec_ctx->width * 3 / 2);

			memset(*data, 0, decoder->video_dec_ctx->height * decoder->video_dec_ctx->width * 3 / 2);

			int height = decoder->video_dec_ctx->height;
			int width = decoder->video_dec_ctx->width;
			printf("decode video ok\n");
			int a = 0, i;
			for (i = 0; i<height; i++)
			{
				memcpy(*data + a, pFrame->data[0] + i * pFrame->linesize[0], width);
				a += width;
			}
			for (i = 0; i<height / 2; i++)
			{
				memcpy(*data + a, pFrame->data[1] + i * pFrame->linesize[1], width / 2);
				a += width / 2;
			}
			for (i = 0; i<height / 2; i++)
			{
				memcpy(*data + a, pFrame->data[2] + i * pFrame->linesize[2], width / 2);
				a += width / 2;
			}
			*size = decoder->i_pkt.size;
			//*pts = decoder->i_pkt.pts;
			ret = 1;
			
		}
		else{
			fprintf(stderr, "decode video frame error!\n");
			goto end;
		}
		
	}
	else if (decoder->i_pkt.stream_index == decoder->audio_stream_index){
		//decode audio
		avcodec_decode_audio4(decoder->audio_dec_ctx, pFrame, &got_picture, &decoder->i_pkt);
		if (got_picture) {
			//fprintf(stderr, "decode one audio frame!\n");
			*data = (uint8_t*)malloc(decoder->i_pkt.size);
			memcpy(*data, pFrame->data, decoder->i_pkt.size);
			
			*size = decoder->i_pkt.size;
			//*pts = decoder_->i_pkt.pts;
			ret = 2;
			
		}
		else{
			fprintf(stderr, "decode audio frame error!\n");
			goto end;
		}
	}
	end:
	av_free_packet(&decoder->i_pkt);
	av_frame_free(&pFrame);
	return ret;

}

// disconnect from remote stream and free AVCodecContext...
void stop_gang_decode(struct gang_decoder* decoder_){
	avformat_close_input(&decoder_->i_fmt_ctx);
}

// free gang_decoder
void free_gang_decode(struct gang_decoder* decoder){
	free(decoder);
}

