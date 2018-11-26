


export default class VideoDecoder {
    constructor(onload) {
        this.onload = onload;
        this.Module = window.Demuxer();
        this.Module["onRuntimeInitialized"] = () => { 
            this.onload();
        };

    }

    init = (data) => {
        console.log(data.length);
        var buffer_p = this.Module._malloc(data.length);
        var video_info_p = this.Module._malloc(4 * 7);
        this.Module.HEAPU8.set(data, buffer_p);
        this.Module._init_muxer(buffer_p, data.length, video_info_p);
        return new Int32Array(this.Module.HEAPU8.buffer, video_info_p, 7);
    }

    get_next = () => {
        let imgBuf, audioLeftBuf, audioRightBuf, sizeBuf, typeBuf;
        imgBuf = this.Module._malloc(4);
        audioLeftBuf = this.Module._malloc(4);
        audioRightBuf = this.Module._malloc(4);
        sizeBuf = this.Module._malloc(4);
        typeBuf = this.Module._malloc(4);
        const ret = this.Module._get_next(imgBuf, audioLeftBuf, audioRightBuf, sizeBuf, typeBuf);
        if(ret < 0) {
            return "done";
        }
        const bufferSize = this.Module.HEAP32[sizeBuf >> 2];
        const bufferType = this.Module.HEAP32[typeBuf >> 2];
        if(bufferType === 1) {
            const leftAudio = new Uint8Array(this.Module.HEAPU8.subarray(audioLeftBuf, audioLeftBuf + bufferSize));
            const rightAudio = new Uint8Array(this.Module.HEAPU8.subarray(audioRightBuf, audioRightBuf + bufferSize));
            this.Module._free(audioLeftBuf);
            this.Module._free(audioRightBuf);
            return {type: "audio", left: leftAudio, right: rightAudio };
        }else {
            const img = new Uint8Array(this.Module.HEAPU8.subarray(imgBuf, imgBuf + bufferSize));
            this.Module._free(imgBuf);
            return {type: "image", img: img };
        }
    }

    setFrame = (frame) => {

    }
} 