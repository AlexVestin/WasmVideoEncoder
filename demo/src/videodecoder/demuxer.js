


export default class VideoDecoder {
    constructor(onload) {
        this.onload = onload;
        this.Module = window.Demuxer8();
        this.Module["onRuntimeInitialized"] = () => { 
            this.onload();
        };

    }

    init = (data) => {
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

        console.log("gettin nwect")
        const ret = this.Module._get_next(imgBuf, audioLeftBuf, audioRightBuf, sizeBuf, typeBuf);
        if(ret < 0) {
            return "done";
        }
        const bufferSize = this.Module.HEAP32[sizeBuf >> 2];
        const bufferType = this.Module.HEAP32[typeBuf >> 2];

        const imgBufPointer = this.Module.HEAP32[imgBuf >> 2];
        const audioLeftBufPointer = this.Module.HEAP32[audioLeftBuf >> 2];
        const audioRightBufPointer = this.Module.HEAP32[audioRightBuf >> 2];

        console.log("buffertype", bufferType);
        if(bufferType === 1) {
            const leftAudio = new Uint8Array(this.Module.HEAPU8.subarray(audioLeftBufPointer, audioLeftBufPointer + bufferSize));
            const rightAudio = new Uint8Array(this.Module.HEAPU8.subarray(audioRightBufPointer, audioRightBufPointer + bufferSize));
            this.Module._free(audioLeftBufPointer);
            this.Module._free(audioRightBufPointer);
            return {type: "audio", left: leftAudio, right: rightAudio };
        }else {
            const img = new Uint8Array(this.Module.HEAPU8.subarray(imgBufPointer, imgBufPointer + bufferSize));
            console.log("FREE???");
            this.Module._free(imgBufPointer);
            console.log("FREE???");
            return {type: "image", img: img };
        }
    }

    setFrame = (frame) => {

    }
} 