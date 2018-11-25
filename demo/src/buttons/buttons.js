import React from 'react';
import PropTypes from 'prop-types';
import { withStyles } from '@material-ui/core/styles';
import Button from '@material-ui/core/Button';
import Options from './options'
import TextField from '@material-ui/core/TextField';

const presetLookup = [
    "ultrafast",
    "veryfast",
    "fast",
    "medium",
    "slow",
    "veryslow"
];

const style= {};

const styles = theme => ({
  paper: {
    marginTop: 30,
    width: theme.spacing.unit * 110,
    backgroundColor: theme.palette.background.paper,
    boxShadow: theme.shadows[5],
    padding: theme.spacing.unit * 4,
  },
});

class SimpleModal extends React.Component {
    constructor(props) {
        super(props)
        this.state = {
            useProjectDuration: true,
            duationValue: props.duration
        }
    }

    handleChange = (e) => {
        const val = e.target.value
        if(!isNaN(val) && val >= 0) {
            this.setState({durationValue: val})
        }else if(val === ""){
            this.setState({durationValue: 0})
        }
    }

    encode = () => {
        if(!this.props.encoding) {
            let sp = this.res.split("x")
            let [w,h] = [Number(sp[0]), Number(sp[1])]
            this.frames = Number(this.fps)
            let br = Number(this.br.slice(0, -1)) * 1000
            let presetIdx = presetLookup.indexOf(this.pre)
            const config = {
                fps: this.frames,
                width: w,
                height: h, 
                duration: Number(this.state.durationValue),
                bitrate: br,
                presetIdx: presetIdx
            }
    
            this.props.startEncoding(config)
        }else {
            this.props.cancelEncoding()
        }
    }

    stopEncoding = ()  => {
        cancelAnimationFrame(this.frameId)
    }

    encoderInit = () => {
        this.startTime = performance.now()
    }

    onEncoderLoaded = () => {
        this.setState({encoderLoaded: true, info: "Module loaded"})
    }

    toggleDurationButton  = () => this.setState({useProjectDuration: !this.state.useProjectDuration})
    
    
    render() {
        const { classes } = this.props;
        const { durationValue } = this.state

        return (
                <div style={style} className={classes.paper}>

                    <div style={{display: "flex", flexDirection: "row"}}>
                        <Options onchange={format => this.format = format} name="format" labels={["mp4"]} disabled></Options>
                        <Options onchange={v => this.res = v} name="resolution" labels={["640x480", "1280x720", "1920x1080"]}></Options>
                        <Options onchange={v => this.fps = v} name="fps" labels={["60"]} disabled></Options>
                        <Options onchange={v => this.br = v} name="bitrate" labels={["1000k", "2000k", "4000k", "6000k", "8000k", "12000k"]}></Options>
                        <Options onchange={v => this.pre = v} name="preset" labels={["ultrafast", "veryfast", "fast", "medium", "slow", "veryslow"]}></Options>
                        <div style={{display: "flex", flexDirection: "row", marginLeft: 30}}>
                            <TextField
                                style={{width: 100, marginTop: 9, marginRight: 15}}
                                label={"duration (sec)"}
                                value={this.state.durationValue}
                                onChange={this.handleChange}
                                type="number"
                                margin="normal"
                            />

                        </div>
                    </div>

                    <Button 
                        variant="contained" 
                        color="primary" 
                        disabled={durationValue < 1 || isNaN(durationValue) || this.props.disabled} 
                        onClick={this.encode}>
                        {"Start encoding"}
                     </Button>
                </div>
        );
    }
}

SimpleModal.propTypes = {
  classes: PropTypes.object.isRequired,
};

// We need an intermediary variable for handling the recursive nesting.
export default  withStyles(styles)(SimpleModal);


