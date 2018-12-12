import * as THREE from 'three';
import VideoDecoder from './demuxer';

export default class Video {
    constructor(file, onready) {
        this.file = file;
        this.onready = onready;

        const fr = new FileReader();
        fr.onload = () => {
            this.bytes = new Uint8Array(fr.result);
            this.loaded = true;
        }

        fr.readAsArrayBuffer(file);
        this.decoder = new VideoDecoder(this.onDecoderReady);
        this.setUpTexture();
    }

    setUpTexture = () => {        
        this.mesh = new THREE.Mesh(
            new THREE.PlaneGeometry(2, 2),
            new THREE.MeshBasicMaterial()
        );
    }

    update = () => {
        if(!this.done) {
            const t0 = performance.now();
            const item = this.decoder.get_next();
            if(item === "done") {
                this.done = true;
                return;
           }

           console.log(performance.now() - t0);
        }
    }

    convertTimeToFrame = (time) => Math.floor((time*this.info.fps) - (this.config.start * this.info.fps));

    setTime = (time, playing) => {
        const frameId = this.convertTimeToFrame(time)
        this.decoder.setFrame(frameId)
    }

    onDecoderReady = () => {
        if(this.loaded) {
            this.info = this.decoder.init(this.bytes);
            this.tex = new THREE.DataTexture(this.texData, this.info.width, this.info.height, THREE.RGBFormat, THREE.UnsignedByteType);
            this.tex.flipY = true;
            this.mesh.material.map = this.tex;
            this.tex.needsUpdate = true;
            this.onready(this.mesh);
        }
    }
}