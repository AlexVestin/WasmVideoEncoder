ENCODE_BC = ffmpeg.bc
ENCODE_DEPS = \
	./build/em/lib/libmp3lame.so \
	./build/em/lib/libx264.so \
	./build/em/lib/libopus.so \
	./build/em/lib/libvpx.so

ENCODE_FILES = ./C/avio_write.c
ENCODE_FUNCTIONS = "[ \
		'_open_audio', '_add_audio_frame', '_add_video_frame', \
		'_open_video', '_close_stream', '_free_buffer', '_get_buffer', \
		'_write_header', '_set_audio']"

ENCODE_COMMON = \
	--llvm-lto 1 \
	-Oz --memory-init-file 0 \
	--closure 1 \
	-o  ../public/WasmEncoder1.js\
	-s WASM=1 \
	-s ALLOW_MEMORY_GROWTH=1 \
	-s EXPORTED_FUNCTIONS=$(ENCODE_FUNCTIONS) \
	-s MODULARIZE=1 \
	-s EXPORT_NAME="'WasmEncoder1'" \
	-s NO_FILESYSTEM=1 \
	-s DISABLE_EXCEPTION_CATCHING=1 \

DECODE_BC = ffmpeg_prod_demux.bc
DECODE_FUNCTIONS ="['_init_muxer', '_get_next_frame', '_set_frame', '_close_muxer']"
DECODE_FILES = ./C/avio_read.c

DECODE_COMMON = \
	-O3 --memory-init-file 0 \
	-o  ../public/Demux2.js\
	-s WASM=1 \
	-s ALLOW_MEMORY_GROWTH=1 \
	-s EXPORTED_FUNCTIONS=$(DECODE_FUNCTIONS) \
	-s MODULARIZE=1 \
	-s EXPORT_NAME="'Demuxer'" \
	-s ASSERTIONS=2

 
demux.js:
	emcc -I./build/em/include -L./build/em/lib $(DECODE_FILES) $(DECODE_COMMON) $(DECODE_BC) $(ENCODE_DEPS) 

encode.js:
	emcc -I./build/em/include $(ENCODE_FILES) $(ENCODE_COMMON) $(ENCODE_BC) $(ENCODE_DEPS) 
