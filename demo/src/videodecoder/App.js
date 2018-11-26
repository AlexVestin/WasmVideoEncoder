import React, { Component } from 'react';
import * as THREE from 'three'
import '../App.css';
import Video from './video'


class App extends Component {
  constructor(props) {
    super(props);
    this.width = 640;
    this.height = 480;

    this.canvasMountRef = React.createRef();
    this.videoLoaded = false;
  }

  componentDidMount() {
    // Threejs renderer set-up
    this.renderer = new THREE.WebGLRenderer({antialias: true, alpha: true});
    this.renderer.setClearColor('#000000');
    this.renderer.setSize(this.width, this.height);
    this.canvasMountRef.current.appendChild(this.renderer.domElement);
    this.scene = new THREE.Scene();
    this.camera = new THREE.OrthographicCamera(-1, 1, 1, -1, 0, 1);
    requestAnimationFrame(this.animate);
  }

  animate = (event) => {
    if(this.videoLoaded) {
      this.video.update();
    }
    this.renderer.render(this.scene, this.camera);
    requestAnimationFrame(this.animate);
  }

  onready = (mesh) => {
    this.scene.add(mesh);
    this.videoLoaded = true;
  }

  loadFile = (event) => {
    const file = event.target.files[0];
    this.video = new Video(file, this.onready);
  }

  render() {
    return (
      <div className="container">
        <div style={{marginTop: 20}} ref={this.canvasMountRef}></div>
        <input type="file" onChange={this.loadFile}></input>
      </div>
    );
  }
}

export default App;
