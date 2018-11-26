ENCODE_BC = ffmpeg.bc
ENCODE_DEPS = \
	./build/em/lib/libmp3lame.so \
	./build/em/lib/libx264.so \
	#./build/em/lib/libopus.so \
	#./build/em/lib/libvpx.so

all: mp4
mp4: ffmpeg.js

ENCODE_FILES = ./C/avio_write.c ./C/extra.c
ENCODE_FUNCTIONS = "[ \
		'_open_audio', '_add_audio_frame', '_add_video_frame', \
		'_open_video', '_close_stream', '_free_buffer', '_write_header']"

ENCODE_COMMON = \
	-O3 \
	-o WasmEncoder1t.js\
	-s EXPORTED_FUNCTIONS=$(ENCODE_FUNCTIONS) \
	-s USE_PTHREADS=1 \
	-s PTHREAD_POOL_SIZE=8 \
	-s ASSERTIONS=0 \
	-s TOTAL_MEMORY=1073741824 \


DECODE_BC = ffmpeg_d.bc
DECODE_FUNCTIONS ="['_init_muxer', '_get_next_frame', '_set_frame', '_close_muxer', '_get_next']"
DECODE_FILES = ./C/avio_read.c


DECODE_COMMON = \
	-O3 --memory-init-file 0 \
	-o  Demux.js\
	-s WASM=1 \
	-s TOTAL_MEMORY=1073741824 \
	-s EXPORTED_FUNCTIONS=$(DECODE_FUNCTIONS) \
	-s MODULARIZE=1 \
	-s EXPORT_NAME="'Demuxer'" \
	-s ASSERTIONS=1

demux.js:
	emcc -I./build/em/include $(DECODE_FILES) $(DECODE_COMMON) $(DECODE_BC) 

encode.js:
	emcc -I./build/em/include $(ENCODE_FILES) $(ENCODE_COMMON) $(ENCODE_BC) $(ENCODE_DEPS) 
