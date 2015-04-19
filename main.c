#include "gang_decoder_impl.h"
#include <stdio.h>

int main(){
	int res;
	int i = 0;
	struct gang_frame* gang_decode_framd;
	struct gang_decoder* decode;
	const char* filename = "rtsp://218.204.223.237:554/live/1/0547424F573B085C/gsfp90ef4k0a6iap.sdp";
	decode = new_gang_decoder(filename);
	
	//filename = "";
	printf("url:%s \n", decode->url);
	res = init_gang_decoder(decode);
	if (!res){
		printf("init error");
	}
	printf("best wdth %d \n", decode->best_width);
	printf("best height %d \n", decode->best_height);
	printf("best fps %d \n", decode->best_fps);
	res = start_gang_decode(decode);
	if (!res){
		printf("start error");
	}
	
	while (i<100){
		gang_decode_framd = gang_decode_next_frame(decode);
		printf("=================read data==============\n");
		printf("size %d\n", gang_decode_framd->size);
		printf("isvideo %d\n", gang_decode_framd->is_video);
		i++;
	}
	stop_gang_decode(decode);
	// free gang_decoder
	free_gang_decode(decode);
}
