


ENCODERS = libmp3lame libx264 aac
MUXERS = mp4 null

SHARED_DEPS = libmp3lame libx264
FFMPEG_PC_PATH = ../../em/lib/pkgconfig

all: ffmpeg

libmp3lame:
	cd ../dependencies/lame && \
	git reset --hard && \
	export CFLAGS="-s USE_PTHREADS=1 -Wno-unknown-warning-option" && \
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
		--extra-cflags="-c -s USE_PTHREADS=1 -Wno-unknown-warning-option" \
		--extra-ldflags="-lpthread" \
		--host=x86-none-linux \
		--disable-cli \
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
		--extra-cflags="-s USE_PTHREADS=1 -pthread -I../../em/include" \
		--extra-ldflags="-Wl,--shared-memory -lpthread -L../../em/lib -s LLD_REPORT_UNDEFINED" \
		&& \
	sed -i 's/HAVE_RINT 0/HAVE_RINT 1/g' config.h && \
	sed -i 's/HAVE_LRINT 0/HAVE_LRINT 1/g' config.h && \
	sed -i 's/HAVE_LRINTF 0/HAVE_LRINTF 1/g' config.h && \
	sed -i 's/HAVE_ROUND 0/HAVE_ROUND 1/g' config.h && \
	sed -i 's/HAVE_ROUNDF 0/HAVE_ROUNDF 1/g' config.h && \
	sed -i 's/HAVE_TRUNC 0/HAVE_TRUNC 1/g' config.h && \
	sed -i 's/HAVE_TRUNCF 0/HAVE_TRUNCF 1/g' config.h && \
	sed -i 's/HAVE_HYPOT 0/HAVE_HYPOT 1/g' config.h && \
	sed -i 's/HAVE_CBRTF 0/HAVE_CBRTF 1/g' config.h && \
	sed -i 's/HAVE_CBRT 0/HAVE_CBRT 1/g' config.h && \
	sed -i 's/HAVE_COPYSIGN 0/HAVE_COPYSIGN 1/g' config.h && \
	sed -i 's/HAVE_ERF 0/HAVE_ERF 1/g' config.h && \
	emmake make -j8 && \
	emmake make install && \
	cp ../../em/bin/ffmpeg ../../../ffmpeg.bc

