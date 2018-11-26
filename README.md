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
These are mostly for comparison with later versions with threads/SIMD  
Firefox 63.0.3  
Chrome 70.0  


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
These are mostly for comparison with later versions with threads/SIMD  
Firefox 63.0.3  
Chrome 70.0  


| settings | Chrome | Firefox | Firefox nightly (threaded) |
| --- | --- | --- | --- |
| 60s 640x480 2000k Ultrafast | 48.1s, 74.9fps, 12.8MB | 35.5s, 101.5fps, 11.8MB | 33.7s, 106.8fps, 11.8MB |
| 60s 640x480 2000k Fast | 148.6s, 24fps, 9.1MB  | 143.8s, 25fps, 7.9MB | 109.7s, 32.8fps, 7.9MB |
| 60s 640x480 2000k Medium | 123.8s, 29fps, 8.9MB  | 106.1s, 33.9fps, 7.7MB | 89.2s, 40.3fps, 7.9MB |
| 60s 1280x720 6000k Ultrafast | 121.5s, 29.6fps, 23MB  | 90.3s, 39.8fps, 21.2MB | 89.1s, 40.4fps, 21.2MB |
| 30s 1280x720 6000k Fast | 203.2s, 8.6fps, 7.8MB   | 202.2s, 8.9fps, 6.6MB | 150.6s, 12.0fps, 6.7MB |
| 30s 1280x720 6000k Medium | 157s, 11.5fps, 7.4MB | 141.5s, 12.7fps, 6.4MB | 117.8s, 15.3fps, 6.5MB |
| 30s 1920x1080 6000k Medium | 336.2s, 5.4fps, 11MB  | 303s,  5.9fps, 9.4MB | - |
| 20s 1920x1080 12000k Ultrafast | 87.7s, 13.7fps, 11.7MB | 64.5s, 18.6fps, 10.7MB | - |
| 20s 1920x1080 12000k Fast | 298.9, 4fps, 7.4MB  | 300.7s, 4fps, 6.5MB | 218.6s, 5.5fps, 6.5MB |

