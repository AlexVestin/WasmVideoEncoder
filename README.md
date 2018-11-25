[Check out a live demo here](http://videoncoder.s3-website.eu-central-1.amazonaws.com/)

# WasmVideoEncoder
Video utils for the browser using FFmpeg with WebAssembly, primarily developed for use in [musicvid.org](https://github.com/alexvestin/musicvid.org).
Currently encodes raw image and audio data, and muxes into an libx264 format (x264 video, mp3 audio).

Example usage in the demo folder, it's the code for the demo website linked earlier.

See ```webassembly/C/avio_write.c``` for the main code of the encoder and ```webassembly/C/avio_read.c``` for the decoder.

Pthreads to be implemented, but will hold off since they aren't very stable at the moment.

### LICENSE
- Code written by me is MIT, the license of the dependencies (ffmpeg, libvpx, lame etc.) varies.

### Build
Makefile & patches from [ffmpeg.js](https://github.com/Kagami/ffmpeg.js/) / [videoconverter.js](https://bgrins.github.io/videoconverter.js/)
```
apt-get update
apt-get -y install wget python git automake libtool build-essential cmake libglib2.0-dev closure-compiler

[[[install and activate emsdk]]]

cd [this repo]
git submodule init
git submodule update --recursive

```

### to run tests without Emscripten:
```
cd build/reg
make 
cd ../../C
LD_LIBRARY_PATH=/WasmVideoEncoder/webassembly/build/reg/lib
PKG_CONFIG_PATH=/WasmVideoEncoder/webassembly/build/reg/lib/pkgconfig
export LD_LIBRARY_PATH
export PKG_CONFIG_PATH
make [your test]
```

### to build for exporting with Emscripten:
```
cd build/em
make
cd ../..
make encode.js
```

# Benchmarks

| settings | Chrome | FireFox |
| --- | --- | --- |
| 60s 640x480 2000k Ultrafast | 48.1s, 74.9fps, 12.8MB | 0 |
| 60s 640x480 2000k Fast | 148.6s, 24fps, 9.1MB  | 0 |
| 60s 640x480 2000k Medium | 123.8s, 29fps, 8.9MB  | 0 |
| 60s 1280x720 6000k Ultrafast | 121.5s, 29.6fps, 23MB  | 0 |
rest tbd.

