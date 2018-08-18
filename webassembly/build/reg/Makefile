


ENCODERS = libx264 libmp3lame aac libvpx_vp8 libopus mjpeg
MUXERS = mp4 mp3 null webm ogg null image2

SHARED_DEPS = libmp3lame libx264 libopus libvpx
FFMPEG_PC_PATH = ../../reg/lib/pkgconfig

all: ffmpeg

build/opus/configure:
	cd ../dependencies/opus && ./autogen.sh

libopus: build/opus/configure
	cd ../dependencies/opus && \
	git reset --hard && \
	./configure \
		CFLAGS=-O3 \
		--prefix="$$(pwd)/../../reg" \
		--disable-static \
		--disable-doc \
		--disable-extra-programs \
		--disable-asm \
		--disable-rtcd \
		--disable-intrinsics \
		&& \
	make -j8 && \
	make install

libvpx:
	cd ../dependencies/libvpx && \
	git reset --hard && \
	./configure \
		--prefix="$$(pwd)/../../reg" \
		--target=generic-gnu \
		--disable-dependency-tracking \
		--disable-multithread \
		--disable-runtime-cpu-detect \
		--enable-shared \
		--disable-static \
		\
		--disable-examples \
		--disable-docs \
		--disable-unit-tests \
		--disable-webm-io \
		--disable-libyuv \
		--disable-vp8-decoder \
		--disable-vp9 \
		&& \
	make -j8 && \
	make install


libmp3lame:
	cd ../dependencies/lame && \
	git reset --hard && \
	patch -p1 < ../../lame-configure.patch && \
	./configure \
		--prefix="$$(pwd)/../../reg" \
		--disable-static \
		\
		--disable-gtktest \
		--disable-analyzer-hooks \
		--disable-decoder \
		--disable-frontend \
		&& \
	make -j8 && \
	make install

libx264:
	cd ../dependencies/x264 && \
	git reset --hard && \
	patch -p1 < ../../x264-configure.patch && \
	./configure \
		--prefix="$$(pwd)/../../reg" \
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
	make -j8 && \
	make install

FFMPEG_COMMON_ARGS = \
	--enable-cross-compile \
	--target-os=none \
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
	PKG_CONFIG_PATH=$(FFMPEG_PC_PATH) ./configure \
		--prefix="$$(pwd)/../../reg" \
		$(FFMPEG_COMMON_ARGS) \
		$(addprefix --enable-encoder=,$(ENCODERS)) \
		$(addprefix --enable-muxer=,$(MUXERS)) \
		--enable-gpl \
		--enable-libmp3lame \
		--enable-libx264 \
		--enable-libopus \
		--enable-libvpx \
		--extra-cflags="-I../../reg/include" \
		--extra-ldflags="-L../../reg/lib" \
		&& \
	make -j8 && \
	make install
