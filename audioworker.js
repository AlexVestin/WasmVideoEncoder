var timerID=null;
var interval=100;

//https://github.com/cwilso/metronome/

onmessage = function(e){
	if (e.data=="start") {
		timerID=setInterval(function(){postMessage("tick");},interval)
	}
	else if (e.data.interval) {
		interval=e.data.interval;
		if (timerID) {
			clearInterval(timerID);
			timerID=setInterval(function(){postMessage("tick");},interval)
		}
	}
	else if (e.data=="stop") {

		clearInterval(timerID);
		timerID=null;
	}
};

