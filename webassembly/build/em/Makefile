


ENCODERS = libx264 libmp3lame aac
MUXERS = mp4 mp3 null null

SHARED_DEPS = libmp3lame libx264
FFMPEG_PC_PATH = ../../em/lib/pkgconfig

all: ffmpeg

libmp3lame:
	cd ../dependencies/lame && \
	git reset --hard && \
	patch -p1 < ../../lame-configure.patch && \
	emconfigure ./configure \
		--prefix="$$(pwd)/../../em" \
		--disable-static \
		\
		--disable-gtktest \
		--disable-analyzer-hooks \
		--disable-decoder \
		--disable-frontend \
		&& \
	emmake make -j8 && \
	emmake make install

libx264:
	cd ../dependencies/x264 && \
	git reset --hard && \
	patch -p1 < ../../x264-configure.patch && \
	emconfigure ./configure \
		--prefix="$$(pwd)/../../em" \
		--extra-cflags="-Wno-unknown-warning-option" \
		--disable-cli \
		--enable-shared \
		--disable-opencl \
		--disable-thread \
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
	\
	--disable-all \
	--enable-ffmpeg \
	--enable-avcodec \
	--enable-avformat \
	--enable-avutil \
	--enable-swresample \
	--enable-swscale \
	--enable-avfilter \
	--disable-network \
	--disable-d3d11va \
	--disable-dxva2 \
	--disable-vaapi \
	--disable-vdpau \
	--disable-protocol=file \
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
	git reset --hard && \
	EM_PKG_CONFIG_PATH=../../em/lib/pkgconfig emconfigure ./configure \
		--prefix="$$(pwd)/../../em" \
		$(FFMPEG_COMMON_ARGS) \
		$(addprefix --enable-encoder=,$(ENCODERS)) \
		$(addprefix --enable-muxer=,$(MUXERS)) \
		--enable-gpl \
		--enable-libmp3lame \
		--enable-libx264 \
		--extra-cflags="-I../../em/include" \
		--extra-ldflags="-L../../em/lib" \
		&& \
	emmake make -j8 && \
	emmake make install && \
	cp ../../em/bin/ffmpeg ../../../ffmpeg.bc

