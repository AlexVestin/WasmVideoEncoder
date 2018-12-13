


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
        let imgBuf = this.Module._malloc(4);
        let audioLeftBuf = this.Module._malloc(4);
        let audioRightBuf = this.Module._malloc(4);
        let sizeBuf = this.Module._malloc(4);
        let typeBuf = this.Module._malloc(4);
        try {
            const ret = this.Module._get_next(imgBuf, audioLeftBuf, audioRightBuf, sizeBuf, typeBuf);
            if(ret === -1) {
                return "done";
            }

            const bufferSize = this.Module.HEAP32[sizeBuf >> 2];
            const bufferType = this.Module.HEAP32[typeBuf >> 2];

            
            if(bufferType === 1) {
                const audioLeftBufPointer = this.Module.HEAP32[audioLeftBuf >> 2];
                const audioRightBufPointer = this.Module.HEAP32[audioRightBuf >> 2];
                const leftAudio = new Float32Array(this.Module.HEAPU8.subarray(audioLeftBufPointer, audioLeftBufPointer + bufferSize).slice(), 0, bufferSize/4);
                const rightAudio = new Float32Array(this.Module.HEAPU8.subarray(audioRightBufPointer, audioRightBufPointer + bufferSize).slice(), 0, bufferSize/4);
                
                this.Module._free(audioLeftBufPointer);
                this.Module._free(audioRightBufPointer);


                this.Module._free(sizeBuf);
                this.Module._free(imgBuf);
                this.Module._free(typeBuf);
                this.Module._free(audioLeftBuf);
                this.Module._free(audioRightBuf);
                return {type: "audio", left: leftAudio, right: rightAudio };
            }else {
                const imgBufPointer = this.Module.HEAP32[imgBuf >> 2];
                const img = new Uint8Array(this.Module.HEAPU8.subarray(imgBufPointer, imgBufPointer + bufferSize));
                this.Module._free(imgBufPointer);

                this.Module._free(sizeBuf);
                this.Module._free(imgBuf);
                this.Module._free(typeBuf);
                this.Module._free(audioLeftBuf);
                this.Module._free(audioRightBuf);
                return {type: "image", img: img };
                }
        }catch(err){
            console.log(err.message)
        }
    }

    setFrame = (frame) => {

    }
} 