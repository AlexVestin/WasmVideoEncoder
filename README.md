# WasmVideoEncoder
Video utils for the browser using FFmpeg with WebAssembly in super early development
Currently encodes raw image and audio data, and muxes into an libx264 format.


Example usage in the demo folder (incoming)
## Example Usage (TODO)


See ```webassembly/C/avio_write.c``` for the main code of the encoder and ```webassembly/C/avio_read.c``` for the decoder.


### LICENSE
- Code written by me is MIT, the license of the dependencies (ffmpeg, libvpx, lame etc.) varies.

### Build
Makefile & patches from [ffmpeg.js](https://github.com/Kagami/ffmpeg.js/) / [videoconverter.js](https://bgrins.github.io/videoconverter.js/)
```
apt-get update
apt-get -y install wget python git automake libtool build-essential cmake libglib2.0-dev closure-compiler

[[[install emsdk]]]

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
