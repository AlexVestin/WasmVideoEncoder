importScripts("WasmEncoder1.js")

let Module = {}
WasmEncoder1(Module)

let usingJS = false
let useAudio = false

const fileType = 1
Module["onRuntimeInitialized"] = () => { 
    postMessage({action: "loaded"})
};

if(usingJS) {
    postMessage({action: "loaded"})
}
openVideo = (config) => {
    let { w, h, fps, bitrate, presetIdx } = config

    Module._open_video(w, h, fps, bitrate, presetIdx, fileType, fileType );
    frameSize = w*h*4
}


let audioFramesRecv = 1,  left, encodeVideo, videoFramesEncoded = 0, audioFramesEncoded = 0
let aduioTimeSum = 0, videoTimeSum = 0
let debug = false

addAudioFrame = (buffer) => {
    const t = performance.now()
    var left_p = Module._malloc(left.length * 4)
    Module.HEAPF32.set(left, left_p >> 2)
    var right_p = Module._malloc(buffer.length * 4)
    Module.HEAPF32.set(buffer, right_p >> 2)
    Module._add_audio_frame(left_p, right_p, left.length)
    const delta = performance.now() - t
    aduioTimeSum+= delta
    if(audioFramesEncoded++ % 25 === 0 && debug) 
        console.log("Audio added, time taken: ", delta, " average: ", aduioTimeSum / audioFramesEncoded)
    postMessage({action: "ready"})
}


openAudio = (config) => {
    const { bitrate, samplerate } = config; 
    try {
      Module._open_audio(samplerate, 2, bitrate, fileType)
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
    var video_p, size_p, size;
    video_p = Module._close_stream(size_p);
    size = Module.HEAP32[size_p >> 2]
    return  new Uint8Array(Module.HEAPU8.subarray(video_p, video_p + size))
}



addVideoFrame = (buffer) => {
    const t = performance.now()
    try {
        var encodedBuffer_p = Module._malloc(buffer.length)
        Module.HEAPU8.set(buffer, encodedBuffer_p)
        Module._add_video_frame(encodedBuffer_p)
    }finally {
        Module._free(encodedBuffer_p)
    }
    //hack to avoid memory leaks
   postMessage(buffer.buffer, [buffer.buffer])
   const delta = performance.now() - t
   videoTimeSum+= delta
   if(videoFramesEncoded++ % 25 === 0 && debug) 
        console.log("Video added, time taken: ", delta, " average: ", videoTimeSum / videoFramesEncoded)
   postMessage({action: "ready"})
}

close = () => {
    let vid = close_stream()
    Module._free_buffer();
    postMessage({action:"return", data: vid.buffer})
}

onmessage = (e) => {
    const { data } = e
    if(data.action === undefined){
        if(encodeVideo) {
            addVideoFrame(data)
        }else {
            if(audioFramesRecv === 1)left = data
            if(audioFramesRecv === 0)addAudioFrame(data)
            audioFramesRecv--;
        }
        return
    }
    
    switch(data.action) {
        case "audio":
            encodeVideo = false;
            audioFramesRecv = 1
            break;
        case "video":
            encodeVideo = true;
            break;
        case "init":
            openVideo(data.data.videoConfig)
            if(data.data.audioConfig !== null){
                openAudio(data.data.audioConfig)
                useAudio = true
            }else {
                useAudio = false
            }
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