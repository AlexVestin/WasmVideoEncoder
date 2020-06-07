ENCODE_BC = ffmpeg.bc
ENCODE_DEPS = \
	./build/em/lib/libx264.a \
	./build/em/lib/libavcodec.a \
	./build/em/lib/libavformat.a \
	./build/em/lib/libswresample.a \
	./build/em/lib/libswscale.a \
	./build/em/lib/libavutil.a \
	./build/em/lib/libavfilter.a \
	./build/em/lib/libmp3lame.a \
	#./build/em/lib/libopus.so \
	#./build/em/lib/libvpx.so

all: mp4
mp4: ffmpeg.js

ENCODE_FILES = ./C/avio_write.c
ENCODE_FUNCTIONS = "[ \
		'_open_audio', '_add_audio_frame', '_add_video_frame', \
		'_open_video', '_close_stream', '_free_buffer', '_write_header', '_free','_get_log_size', '_get_log']"

ENCODE_SEQUENTIAL_COMMON = \
	-O3 \
	-o WasmEncoder1s.js\
	-s EXPORTED_FUNCTIONS=$(ENCODE_FUNCTIONS) \
	-s ALLOW_MEMORY_GROWTH=1 \
	-s MODULARIZE=1 \
	-s EXPORT_NAME="WasmEncoder" \
	-s NO_FILESYSTEM=1 \


ENCODE_THREADED_COMMON = \
	-O3 \
	-o WasmEncoder1t.js\
	-s EXPORTED_FUNCTIONS=$(ENCODE_FUNCTIONS) \
	-s ALLOW_MEMORY_GROWTH=1 \
	-s MODULARIZE=1 \
	-s EXPORT_NAME="WasmEncoder" \
	-s USE_PTHREADS=1 \
	-s ALLOW_BLOCKING_ON_MAIN_THREAD=1 \
	-s NO_FILESYSTEM=1 \

DECODE_BC = ffmpeg_dec.bc
DECODE_FUNCTIONS ="['_init_muxer', '_get_next_frame', '_set_frame', '_close_muxer', '_get_next']"
DECODE_FILES = ./C/avio_read.c

MALLOC_TEST = \
	-s EXPORTED_FUNCTIONS="['_test_malloc']" \
	-s MODULARIZE=1 \
	-s EXPORT_NAME="'MallocTest'" \
	-s ALLOW_MEMORY_GROWTH=1 \
	-o malloc.js

DECODE_COMMON = \
	-O3 \
	--enable-bulk-memory \
	-o  Demux.js\
	-s WASM=1 \
	-s ALLOW_MEMORY_GROWTH=1 \
	-s EXPORTED_FUNCTIONS=$(DECODE_FUNCTIONS) \
	-s MODULARIZE=1 \
	-s NO_FILESYSTEM=1\
	-s EXPORT_NAME="'Demuxer8'" \
	-s TOTAL_MEMORY=1024MB


demux.js:
	emcc -I./build/em/include $(DECODE_FILES) $(DECODE_COMMON) $(DECODE_BC) 

sequential:
	emcc -I./build/em/include $(ENCODE_FILES) $(ENCODE_THREADED_COMMON) $(ENCODE_THREADED_BC) $(ENCODE_DEPS) && \
	./install_win.sh

encode.js:
	emcc -I./build/em/include $(ENCODE_FILES) $(ENCODE_THREADED_COMMON) $(ENCODE_THREADED_BC) $(ENCODE_DEPS) && \
	./install_win.sh

malloc.js:
	emcc -I./build/em/include malloc_test.c $(MALLOC_TEST) 