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

        this.framesRead = 0;
    }

    setUpTexture = () => {        
        this.mesh = new THREE.Mesh(
            new THREE.PlaneGeometry(2, 2),
            new THREE.MeshBasicMaterial({color: "green"})
        );
    }

    update = () => {
        if(!this.done) {
            const item = this.decoder.get_next();
            if(item === "done") {
                this.done = true;
                return;
           }

           if(item.type === "video") {
                this.texData.set(item.img);
                this.tex.needsUpdate = true;
           }
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
            this.texData = new Uint8Array(this.info.width * this.info.height * 3);
            this.tex = new THREE.DataTexture(this.texData, this.info.width, this.info.height, THREE.RGBFormat, THREE.UnsignedByteType);
            this.tex.generateMipmaps = false;
            this.tex.minFilter = THREE.LinearFilter;
            this.tex.flipY = true;
            this.mesh.material.map = this.tex;
            this.tex.needsUpdate = true;
            this.onready(this.mesh);
        }
    }
}