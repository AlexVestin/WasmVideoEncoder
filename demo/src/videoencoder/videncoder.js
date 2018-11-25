//import WasmVideoEncoder from './WasmVideoEncoder'

export default class VideoEncoder {
    constructor(onload){
        this.onload = onload;
        this.Module = {};
         
        window.Module["onRuntimeInitialized"] = () => { 
            this.onload();
        };
    }

    init = (videoConfig, audioConfig, oninit, getFrame) => {
        let { width, height, fps, bitrate, presetIdx } = videoConfig;
        window.Module._open_video(width, height, fps, bitrate, presetIdx, 1, 1);
        window.Module._write_header();
        this.getFrame = getFrame;
    }

    sendFrame = (obj) => {
        const buffer = obj.pixels;
        try {
            var encodedBuffer_p = window.Module._malloc(buffer.length);
            window.Module.HEAPU8.set(buffer, encodedBuffer_p);
            window.Module._add_video_frame(encodedBuffer_p);
        }finally {
            window.Module._free(encodedBuffer_p);
        }
    }

    close_stream = () => {
        var video_p, size_p, size;
        video_p = window.Module._close_stream(size_p);
        size = window.Module.HEAP32[size_p >> 2];
        return new Uint8Array(window.Module.HEAPU8.subarray(video_p, video_p + size));
    }
 
    close = () => {
        let vid = this.close_stream()
        window.Module._free_buffer();
        return vid;
    }
}