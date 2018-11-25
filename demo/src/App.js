import React, { Component } from 'react';
import * as THREE from 'three'
import * as FileSaver from "file-saver";
import './App.css';
import ButtonGroup from './buttons/buttons'
import VideoEncoder from './videoencoder/videncoder'


class App extends Component {
  constructor(props) {
    super(props);
    this.width = 640;
    this.height = 480;

    this.state = {encoderLoaded: false, encoding: false, frames: 0}; 

    this.videoEncoder = new VideoEncoder(this.videoEncoderLoaded);
    this.canvasMountRef = React.createRef();
  }

  componentDidMount() {
    // Threejs renderer set-up
    this.renderer = new THREE.WebGLRenderer({antialias: true, alpha: true});
    this.renderer.setClearColor('#000000');
    this.renderer.setSize(this.width, this.height);
    this.canvasMountRef.current.appendChild(this.renderer.domElement);
    this.setUpScene();
  }

  setUpScene = () => {
    this.scene = new THREE.Scene();
    this.camera = new THREE.PerspectiveCamera();
    var geometry = new THREE.BoxGeometry( 1, 1, 1 );
    var material = new THREE.MeshBasicMaterial( { color: 0x00ff00 } );
    
    this.cube = new THREE.Mesh( geometry, material );
    this.scene.add( this.cube );
    this.camera.position.z = 5;
    this.renderScene();
  }

  renderScene = () => {
    if(!this.state.encoding) {
      this.animate();
      requestAnimationFrame(this.renderScene);
    }
  }

  animate = () => {
    this.cube.rotation.x += 0.01;
    this.cube.rotation.y += 0.01;
    this.renderer.render(this.scene,this.camera);
  }

  videoEncoderLoaded = () => {
    this.setState({encoderLoaded: true});
  }

  getFrame = () => {
    if(this.frames < this.duration * this.fps) {
      // animate the scene
      this.animate();
    
      // read pixels from scene into a buffer
      this.pixels = new Uint8Array(this.width * this.height * 4);
      const gl = this.renderer.getContext();
      gl.readPixels(0,0,this.width,this.height, gl.RGBA, gl.UNSIGNED_BYTE, this.pixels);

      // queue in videoencoder
      this.videoEncoder.sendFrame({type: "video", pixels: this.pixels});
      this.frames++;
      if(this.frames % 60 === 0)
        this.setState({frames: this.frames});
      
        requestAnimationFrame(this.getFrame);
    }else {
      this.saveToFile(this.videoEncoder.close());
      this.renderer.setSize(640, 480);
      this.setState({frames: 0, encoding: false});
      this.renderScene();
    }

    
  }
  saveToFile = (vid) => {
    const s = (performance.now() - this.startTime) / 1000
    console.log(s + "s and " +  this.duration / s * this.fps + "fps" );
    FileSaver.saveAs(new Blob([vid], { type: 'video/mp4' }), "vid.mp4")
  }

  // initialize the video encoding
  startEncoding = (config) => {
    this.duration = config.duration;
    this.fps = config.fps;
    this.width = config.width;
    this.frames = 0;
    this.height = config.height;
    this.renderer.setSize(config.width, config.height);
    this.setState({encoding: true});
    // second parameter is audioConfig which looks like audioConfig: { birate: xxxxx, sampleRate: xxxxxx }
    // third is an oninit callback, if you want to prepare 
    this.videoEncoder.init(config, null, null, this.getFrame);
    this.getFrame();
    this.startTime = performance.now();
  }

  render() {
    const { encoderLoaded, encoding } = this.state;
    return (
      <div className="container">
        <div style={{marginTop: 20, display: this.state.encoding ? "none" : ""}} ref={this.canvasMountRef}></div>
        {this.state.encoding && <React.Fragment>{this.state.frames} / {this.duration * this.fps} frames encoded</React.Fragment>}
        <ButtonGroup disabled={!encoderLoaded && encoding} startEncoding={this.startEncoding}></ButtonGroup>
      </div>
    );
  }
}

export default App;
