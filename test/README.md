gang_rec_only_test_main

gcc gang_decoder_impl.c ffmpeg_format.c ffmpeg_transcoding.c test/gang_rec_only_test_main.c -o test/gang_rec_only_test_main -Wall -g -I/home/savage/git/macro-logger -I. -L/home/savage/soft/webrtc/webrtc-linux64/lib/Release `pkg-config --libs nss x11 libavcodec libavformat libavfilter libswresample` -lwebrtc_full -std=c99 -lX11 -lpthread -lrt -ldl