#include "gang_decoder_impl.h"
#include <stdio.h>

int main(){
	int res;
	int i = 0;
	int width = 0;
	int heith = 0;
	int fps = 0;
	int* best_width = &width;
	int* best_height = &heith;
	int* best_fps = &fps;
	int temp_size = 0;
	int * size = &temp_size;

	struct gang_decoder* decode;
	uint8_t ** data = (uint8_t**)malloc(sizeof(uint8_t*));
	const char* filename = "rtsp://218.204.223.237:554/live/1/0547424F573B085C/gsfp90ef4k0a6iap.sdp";
	decode = new_gang_decoder(filename);

	printf("url:%s \n", decode->url);

	res = init_gang_decoder(decode, best_width, best_height, best_fps);
	if (!res){
		printf("init error");
	}	
	printf("best wdth %d \n", *best_width);
	printf("best height %d \n", *best_height);
	printf("best fps %d \n", *best_fps);
	res = start_gang_decode(decode);
	if (!res){
		printf("start error");
	}

	while (i<1000){
		res = gang_decode_next_frame(decode, data, size);
		printf("=================read data==============\n");
		printf("size %d\n", *size);
		printf("res %d\n", res);
		i++;
		//if (*data!=NULL)
		//free(*data);
	}
	stop_gang_decode(decode);
	// free gang_decoder
	free_gang_decode(decode);
	
}