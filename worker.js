importScripts("WasmVideoEncoderTest2.js")

let Module = {}
WasmVideoEncoderTest2(Module)

let encodedFrames = 0
let initialized = false
let startTime, frameSize

Module["onRuntimeInitialized"] = () => { 
    postMessage({action: "loaded"})
};

openVideo = (config) => {
    let { w, h, fps, bitrate, presetIdx } = config
    Module._open_video(w, h, fps, bitrate, presetIdx);

    frameSize = w*h*4
}

openAudio = (config) => {
    let { bitrate, left, right, samplerate, duration } = config; 
    let durationInBytes = Math.floor(duration * samplerate)
    left = left.subarray(0, durationInBytes)
    right = right.subarray(0, durationInBytes)
    
    try {
      var left_p = Module._malloc(left.length * 4)
      Module.HEAPF32.set(left, left_p >> 2)
      
      var right_p = Module._malloc(right.length * 4)
      Module.HEAPF32.set(right, right_p >> 2)
      Module._open_audio(left_p, right_p, left.length, samplerate, 2, bitrate)

    }catch(err) {
      console.log(err)
    }
}

writeHeader = () => {
    Module._write_header();
    postMessage({action: "initialized"})
    initialized = true
} 

close_stream = () => {
    var video_p, size;
    size = Module._close_stream();
    video_p = Module._get_buffer();
    return  new Uint8Array(Module.HEAPU8.subarray(video_p, video_p + size))
}

addFrame = (buffer) => {
    let nrFrames = buffer.length / frameSize
    try {
        var encodedBuffer_p = Module._malloc(buffer.length)
        Module.HEAPU8.set(buffer, encodedBuffer_p)
        Module._add_frame(encodedBuffer_p, nrFrames)
    }finally {
        Module._free(encodedBuffer_p)
        encodedFrames++;
    }
    //hack to avoid memory leaks
   postMessage(buffer.buffer, [buffer.buffer])
   postMessage({action: "ready"})
}

close = () => {
    Module._write_audio_frame()
    let vid = close_stream()
    Module._free_buffer();
    postMessage({action:"return", data: vid.buffer})
}

onmessage = (e) => {
    const { data } = e
    if(data.action === undefined){
        addFrame(data)
        return
    }
    
    switch(data.action) {
        case "init":
            openVideo(data.data.videoConfig)
            openAudio(data.data.audioConfig)
            writeHeader()
            postMessage({action: "initialized"})
            break;
        case "addFrame":
            addFrame(data.data)
            break;
        case "close":
            close(data.data)
            break;
        default:
            console.log("unknown command")
    }
}