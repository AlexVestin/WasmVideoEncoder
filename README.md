# WasmVideoEncoder
Video utils for the browser using FFmpeg with WebAssembly in super early development
Currently encodes raw image and audio data, and muxes into an libx264 format.


Example usage in the demo folder (incoming)
## Example Usage (TODO)


See ```webassembly/C/avio_write.c``` for the main code of the encoder and ```webassembly/C/avio_read.c``` for the decoder.



### TODOs
- Add working example
- ~~Move videoediting stuff to new repo~~
- ~~Fix/Add build to project~~ (src/webassembly/build/em)
- ~~Better animations demo~~
- ~~Custom sound option for demo~~
- ~~Fix worker memory usage ( reads the canvas faster than it encodes -> frames stack up -> memory out of bounds )~~
- ~~~smaller wasm/js~~
- better README
  - benchmarks
  - images
- ~~h265/vp9/vp8/other codec support~~ (sort of)
- ~~interleaved audio/video writing~~
- ~~C code improvements (memory management, simplify code)~~

### LICENSE
- Code written by me is MIT, the license of the dependencies (ffmpeg, libvpx, lame etc.) varies.

### Build
Makefile & patches from [ffmpeg.js}(https://github.com/Kagami/ffmpeg.js/) / [videoconverter.js](https://bgrins.github.io/videoconverter.js/)
```
apt-get update
apt-get -y install wget python git automake libtool build-essential cmake libglib2.0-dev closure-compiler

cd /root # or /home/whatever
wget https://s3.amazonaws.com/mozilla-games/emscripten/releases/emsdk-portable.tar.gz
tar xzvf emsdk-portable.tar.gz
cd emsdk-portable
./emsdk update
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

cd /root # or /home/whatever
git clone https://github.com/Kagami/ffmpeg.js.git
cd ffmpeg.js
git submodule init
git submodule update --recursive

make all

```
