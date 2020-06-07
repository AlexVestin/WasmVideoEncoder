


ENCODERS = libmp3lame libx264 aac
MUXERS = mp4 null

SHARED_DEPS = libmp3lame libx264
FFMPEG_PC_PATH = ../../em/lib/pkgconfig

all: ffmpeg

libmp3lame:
	cd ../dependencies/lame && \
	git reset --hard && \
	export CFLAGS="-Wno-unknown-warning-option" && \
	emconfigure ./configure \
		--prefix="$$(pwd)/../../em" \
		--enable-static \
		--host=x86-none-linux \
		--disable-gtktest \
		--disable-analyzer-hooks \
		--disable-decoder \
		--disable-frontend \
		&& \
	emmake make -j8 && \
	emmake make install

libx264:
	cd ../dependencies/x264 && \
	emconfigure ./configure \
		--prefix="$$(pwd)/../../em" \
		--extra-cflags="-c -Wno-unknown-warning-option" \
		--extra-ldflags="-lpthread" \
		--host=x86-none-linux \
		--disable-cli \
		--disable-thread \
		--enable-static \
		--disable-shared \
		--disable-opencl \
		--disable-interlaced \
		--bit-depth=8 \
		--chroma-format=420 \
		--disable-asm \
		\
		--disable-avs \
		--disable-swscale \
		--disable-lavf \
		--disable-ffms \
		--disable-gpac \
		--disable-lsmash \
		&& \
	emmake make -j8 && \
	emmake make install

FFMPEG_COMMON_ARGS = \
	--cc=emcc \
	--ranlib=emranlib \
	--enable-cross-compile \
	--target-os=none \
	--arch=x86 \
	--disable-runtime-cpudetect \
	--disable-asm \
	--disable-fast-unaligned \
	--disable-pthreads \
	--disable-w32threads \
	--disable-os2threads \
	--disable-debug \
	--disable-stripping \
	--disable-safe-bitstream-reader \
	\
	--disable-all \
	--enable-ffmpeg \
	--enable-avcodec \
	--enable-avformat \
	--enable-avfilter \
	--enable-swresample \
	--enable-swscale \
	--disable-network \
	--disable-d3d11va \
	--disable-dxva2 \
	--disable-vaapi \
	--disable-vdpau \
	--enable-protocol=file \
	--disable-bzlib \
	--disable-iconv \
	--disable-libxcb \
	--disable-lzma \
	--disable-sdl2 \
	--disable-securetransport \
	--disable-xlib \
	--disable-zlib

ffmpeg: $(SHARED_DEPS)
	cd ../dependencies/ffmpeg && \
	EM_PKG_CONFIG_PATH=../../em/lib/pkgconfig emconfigure ./configure \
		--prefix="$$(pwd)/../../em" \
		$(FFMPEG_COMMON_ARGS) \
		$(addprefix --enable-encoder=,$(ENCODERS)) \
		$(addprefix --enable-muxer=,$(MUXERS)) \
		--enable-gpl \
		--enable-libx264 \
		--enable-libmp3lame \
		--extra-cflags="-I../../em/include" \
		--extra-ldflags="-L../../em/lib -s LLD_REPORT_UNDEFINED" \
		&& \
	emmake make -j8 && \
	emmake make install && \
	cp ../../em/bin/ffmpeg ../../../ffmpeg_seq.bc

