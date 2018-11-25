//import WasmVideoEncoder from './WasmVideoEncoder'

export default class VideoEncoder {
    constructor(onload){

        // in the public folder
        this.worker = new Worker("encodeworker.js");
        this.worker.onmessage = this.onmessage;

        this.onload = onload;
        this.isWorker = true;

        this.frames = [];
        this.buffer = new Uint8Array();
    }

    init = (videoConfig, audioConfig, oninit, getFrame) => {
        this.worker.postMessage({action: "init", data: {audioConfig, videoConfig}});
        this.oninit = oninit;
        this.getFrame = getFrame;
        this.closed = false;
    }

    sendFrame = () => {
        const frame = this.frames.pop();
        if(frame && !this.closed) {
            this.worker.postMessage({action: frame.type})
            if(frame.type === "audio") {
                this.worker.postMessage(frame.left, [frame.left.buffer])
                this.worker.postMessage(frame.right, [frame.right.buffer])
            }else {
                this.worker.postMessage(frame.pixels, [frame.pixels.buffer])
            }   
        }
    }

    queueFrame = (frame) => {
        this.frames.push(frame)
    }

    close = (onsuccess) => {
        this.closed = true
        this.onsuccess = onsuccess
        setTimeout(e => this.worker.postMessage({action: "close"}), 500)                        
    }
    
    onmessage = (e) => {
        const { data } = e;
        switch(data.action){
            case "loaded":
                this.onload()
                break;
            case "initialized":
                if(this.oninit)
                    this.oninit();
                this.getFrame();
                this.sendFrame();
                break;
            case "ready":
                if(!this.closed) {
                    this.getFrame();
                    this.sendFrame();
                }
                break;
            case "return":
                this.onsuccess(data.data)
                break;
            case "error":
                break;
            default:
        }
    }
}