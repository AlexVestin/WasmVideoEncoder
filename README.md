# WasmVideoEncoder
Video utils for the browser using FFmpeg with WebAssembly in super early development

Currently encodes raw image and audio data, and muxes into an libx264 format.

Example usage in the demo folder (incoming)

## Example Usage
```
import VideoEncoder from './VideoEncoder/VideoEncoderWorker'

let framesEncoded = 0
let saveVideoCallback = (file) => {
  const blob = new Blob([file], { type: 'video/mp4' });
  window.navigator.msSaveOrOpenBlob(blob);
}

let queueFrame = () => {
    animate()
    let pixels = readPixels() // (gl.readPixels or ctx.getImageData)
    videoEncoder.queueFrame( {type: "video", pixels: pixels} )
    
    if(++framesEncoded > 250) {
      videoEncoder.stop(saveVideoCallback)
    }
}

let onInitialized = () => {
  queueFrame()
}

let encoderLoadedCallback = () => {
    let videoConfig = {
        w: 720,
        h: 480,
        fps: 25,
        bitrate: 400000,
        presetIdx: 0 // ultrafast preset
    }
    videoEncoder.init(videoConfig, undefined /* audioconfig */, onInitialized, queueFrame /* will be called when the encoder is ready */)
}

videoEncoder = new VideoEncoder(encoderLoadedCallback)
```


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
Makefile & patches from ffmpeg.js / videoencoder.js

