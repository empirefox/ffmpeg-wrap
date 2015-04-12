#include <stdlib.h>
#include "gang_decoder.h"

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
	decoder->best_width = 0;
	decoder->best_height = 0;
	decoder->best_fps = 20;
	decoder->video_stream_index = -1;
	decoder->audio_stream_index = -1;

	decoder->i_fmt_ctx     = NULL;
	decoder->video_dec_ctx = NULL;
	decoder->audio_dec_ctx = NULL;

	return decoder;
}

int init_gang_decoder(struct gang_decoder* decoder_){
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

	if (avformat_open_input(&i_fmt_ctx, decoder_->url, NULL, NULL) != 0)
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
		decoder_->best_width = video_dec_ctx->width;
		decoder_->best_height = video_dec_ctx->height;
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

struct gang_frame* gang_decode_next_frame(struct gang_decoder* decoder_){

	
	if (av_read_frame(decoder_->i_fmt_ctx, &decoder_->i_pkt) < 0){
		fprintf(stderr, "av_read_frame error!\n");
	}
	AVFrame *pFrame = avcodec_alloc_frame();
	int got_picture = 0, ret = 0;
	struct gang_frame*  gang_read_frame = (struct gang_frame*)malloc(sizeof(struct gang_frame));

	//video
	if (decoder_->i_pkt.stream_index == decoder_->video_stream_index) {

		avcodec_decode_video2(decoder_->video_dec_ctx, pFrame, &got_picture, &decoder_->i_pkt);
		if (got_picture) {

			gang_read_frame->data = malloc(decoder_->video_dec_ctx->height * decoder_->video_dec_ctx->width * 3 / 2);

			memset(gang_read_frame->data, 0, decoder_->video_dec_ctx->height * decoder_->video_dec_ctx->width * 3 / 2);

			int height = decoder_->video_dec_ctx->height;
			int width = decoder_->video_dec_ctx->width;
			printf("decode video ok\n");
			int a = 0, i;
			for (i = 0; i<height; i++)
			{
				memcpy(gang_read_frame->data + a, pFrame->data[0] + i * pFrame->linesize[0], width);
				a += width;
			}
			for (i = 0; i<height / 2; i++)
			{
				memcpy(gang_read_frame->data + a, pFrame->data[1] + i * pFrame->linesize[1], width / 2);
				a += width / 2;
			}
			for (i = 0; i<height / 2; i++)
			{
				memcpy(gang_read_frame->data + a, pFrame->data[2] + i * pFrame->linesize[2], width / 2);
				a += width / 2;
			}
			gang_read_frame->size = decoder_->i_pkt.size;
			gang_read_frame->pts = decoder_->i_pkt.pts;
			gang_read_frame->is_video = 1;
			gang_read_frame->status = 1;
		}
		else{
			fprintf(stderr, "decode video frame error!\n");
			gang_read_frame->data = NULL;
			gang_read_frame->size = 0;
			gang_read_frame->pts = 0;
			gang_read_frame->is_video = 1;
			gang_read_frame->status = 0;
		
		}
		
	}else if (decoder_->i_pkt.stream_index == decoder_->audio_stream_index){
		//decode audio
		avcodec_decode_audio4(decoder_->audio_dec_ctx, pFrame, &got_picture, &decoder_->i_pkt);
		if (got_picture) {
			//fprintf(stderr, "decode one audio frame!\n");
			gang_read_frame->data = (uint8_t*)malloc(decoder_->i_pkt.size);
			memcpy(gang_read_frame->data, pFrame->data, decoder_->i_pkt.size);
			
			gang_read_frame->size = decoder_->i_pkt.size;
			gang_read_frame->pts = decoder_->i_pkt.pts;
			gang_read_frame->is_video = 0;
			gang_read_frame->status = 1;
		}
		else{
			fprintf(stderr, "decode audio frame error!\n");

			gang_read_frame->data = NULL;
			gang_read_frame->size = 0;
			gang_read_frame->pts = 0;
			gang_read_frame->is_video = 0;
			gang_read_frame->status = 0;
		}
	}
	av_free_packet(&decoder_->i_pkt);
	avcodec_free_frame(&pFrame);
	return gang_read_frame;

}
void free_gang_frame(struct gang_frame* gang_decode_frame){
	if (gang_decode_frame->data!=NULL)
	free(gang_decode_frame->data);
	if (gang_decode_frame != NULL){
		free(gang_decode_frame);
	}
}
// disconnect from remote stream and free AVCodecContext...
void stop_gang_decode(struct gang_decoder* decoder_){
	avformat_close_input(&decoder_->i_fmt_ctx);
}

// free gang_decoder
void free_gang_decode(struct gang_decoder* decoder_){
	free(decoder_);
}
