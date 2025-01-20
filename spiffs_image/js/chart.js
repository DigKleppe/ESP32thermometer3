var chartData;
var chartRdy = false;
var tick = 0;
var dontDraw = false;
var halt = false;
var chartHeigth = 500;
var simValue1 = 0;
var simValue2 = 0;
var table;
var presc = 1;
var simMssgCnts = 0;
var lastTimeStamp = 0;
var REQINTERVAL = 30; // sec
var firstRequest = true;
var plotTimer = 6; // every 60 seconds plot averaged value
var rows = 0;
var measTimeLastSample;


var MINUTESPERTICK = 5;// log interval 
var LOGDAYS = 7;
var MAXPOINTS = LOGDAYS * 24 * 60 / MINUTESPERTICK;

var displayNames = ["", "t1", "t2", "t3", "t4"];
var cbIDs = ["", "T1cb", "T2cb", "T3cb", "T4cb"];
var chartSeries = [-1, -1, -1, -1, -1];

var NRItems = displayNames.length;

var dayNames = ['zo', 'ma', 'di', 'wo', 'do', 'vr', 'za'];
var options = {
	title: '',
	curveType: 'function',
	legend: { position: 'bottom' },

	heigth: 200,
	crosshair: { trigger: 'both' },	// Display crosshairs on focus and selection.
	explorer: {
		actions: ['dragToZoom', 'rightClickToReset'],
		//actions: ['dragToPan', 'rightClickToReset'],
		axis: 'horizontal',
		keepInBounds: true,
		maxZoomIn: 100.0
	},
	chartArea: { 'width': '90%', 'height': '60%' },

	vAxes: {
		0: { logScale: false },
	},
	/*	series: {
			0: { targetAxisIndex: 0 },// T1
			1: { targetAxisIndex: 0 },// T2
			2: { targetAxisIndex: 0 },// T3
			3: { targetAxisIndex: 0 },// T4
			4: { targetAxisIndex: 0 },// Tref
		},*/
};

function clear() {
	chartData.removeRows(0, chartData.getNumberOfRows());
	chart.draw(chartData, options);
	tick = 0;
}

//var formatter_time= new google.visualization.DateFormat({formatType: 'long'});
// channel 1 .. 5


function plot(channel, value, timeStamp) {
	if (chartRdy) {
		if (channel == 0) {
			chartData.addRow();
			if (chartData.getNumberOfRows() > MAXPOINTS == true)
				chartData.removeRows(0, chartData.getNumberOfRows() - MAXPOINTS);
			var date = new Date(timeStamp);
			var labelText = date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds();
			chartData.setValue(chartData.getNumberOfRows() - 1, 0, labelText);
		}
		else {
			value = parseFloat(value); // from string to float
			chartData.setValue(chartData.getNumberOfRows() - 1, channel, value);
		}
	}
}



function plotTemperatureOld(channel, value) {
	if (chartRdy) {
		//		if (channel == 1) {
		//			tempData.addRow();
		//			if (tempData.getNumberOfRows() > MAXPOINTS == true)
		//				tempData.removeRows(0, tempData.getNumberOfRows() - MAXPOINTS);
		//		}
		if (value != '--') {
			if (value > -50.0) {
				value = parseFloat(value); // from string to float
				chartData.setValue(chartData.getNumberOfRows() - 1, channel, value);
			}
		}
	}
}

function loadCBs() {
	var cbstate;

	console.log("Reading CBs");

	// Get the current state from localstorage
	// State is stored as a JSON string
	cbstate = JSON.parse(localStorage['CBState'] || '{}');

	// Loop through state array and restore checked 
	// state for matching elements
	for (var i in cbstate) {
		var el = document.querySelector('input[name="' + i + '"]');
		if (el) el.checked = true;
	}

	// Get all checkboxes that you want to monitor state for
	var cb = document.getElementsByClassName('save-cb-state');

	// Loop through results and ...
	for (var i = 0; i < cb.length; i++) {

		//bind click event handler
		cb[i].addEventListener('click', function (evt) {
			// If checkboxe is checked then save to state
			if (this.checked) {
				cbstate[this.name] = true;
			}

			// Else remove from state
			else if (cbstate[this.name]) {
				delete cbstate[this.name];
			}

			// Persist state
			localStorage.CBState = JSON.stringify(cbstate);
		});
	}
	console.log("CBs read");
	initTimer();
};


function initChart() {
	window.addEventListener('load', loadCBs());
}

function initTimer() {
	var activeSeries = 1;
	simValue1 = 0;

	chart = new google.visualization.LineChart(document.getElementById('chart'));
	chartData = new google.visualization.DataTable();
	chartData.addColumn('string', 'Time');

	for (var m = 1; m < NRItems; m++) { // time not used for now 
		var cb = document.getElementById(cbIDs[m]);
		if (cb) {
			if (cb.checked) {
				chartData.addColumn('number', displayNames[m]);
				chartSeries[m] = activeSeries;
				activeSeries++;
			}
		}
	}
	if (activeSeries == 1) {
		alert("No channels selected");
		chartData.addColumn('number', displayNames[1]);
		chartSeries[1] = 1;
	}

	chartRdy = true;
	if (SIMULATE) {
		simplot();
	}
	else {
		setInterval(function () { timer() }, 1000);
	}
}


function updateLastDayTimeLabel(data) {
	var ms = Date.now();
	var date = new Date(ms);
	var labelText = date.getHours() + ':' + date.getMinutes();
	data.setValue(data.getNumberOfRows() - 1, 0, labelText);
}

function updateAllDayTimeLabels(data) {
	var rows = data.getNumberOfRows();
	var minutesAgo = rows * MINUTESPERTICK;
	var ms = Date.now();
	ms -= (minutesAgo * 60 * 1000);
	for (var n = 0; n < rows; n++) {
		var date = new Date(ms);
		var labelText = dayNames[date.getDay()] + ';' + date.getHours() + ':' + date.getMinutes();
		data.setValue(n, 0, labelText);
		ms += 60 * 1000 * MINUTESPERTICK;

	}
}

function simplot() {
	var str = "";
	var str2 = "";
	for (var n = 0; n < 20; n++) {
		simValue1 += 100; // timestamp
		simValue2 = Math.sin(simValue1 * 0.001);
		str = simValue1 + "," + (simValue2 + 1) + "," + (simValue2 + 2) + "," + (simValue2 + 3) + "," + (simValue2 + 4) + ",\n";
		str2 = str + str2;
	}
	plotArray(str2);

	for (var m = 1; m < NRItems; m++) {
		var value = simValue2; // from string to float
		document.getElementById(displayNames[m]).innerHTML = value.toFixed(2);
	}
}

function plotArray(str) {
	var arr;
	var arr2 = str.split("\n");
	var nrPoints = arr2.length - 1;
	var timeOffset;
	if (nrPoints > 0) {

		arr = arr2[nrPoints - 1].split(",");
		measTimeLastSample = arr[0];  // can be unadjusted time in 10 ms units
		//		document.getElementById('valueDisplay').innerHTML = arr[1] + " " + arr[2]; // value of last measurement

		var sec = Date.now();//  / 1000;  // mseconds since 1-1-1970 

		timeOffset = sec - parseFloat(measTimeLastSample) * 10;

		for (var p = 0; p < nrPoints; p++) {
			arr = arr2[p].split(",");
			if (arr.length >= 5) {
				sampleTime = sec - parseFloat(arr[0]) * 10 + timeOffset; //arr[0] is time in 10 ms units since start of program
				plot(0, arr[1], sampleTime);  // new point

				for (var m = 1; m < NRItems; m++) {
					if (chartSeries[m] != -1)
						plot(chartSeries[m], arr[m], sampleTime);
				}
			}
		}
		chart.draw(chartData, options);
	}
}


function timer() {
	var arr;
	if (SIMULATE) {
		simplot();
	}
	else {
		if (!halt) {
			if (firstRequest) {
				arr = getAllLogs();
				firstRequest = false;
				setInterval(function() { timer() }, 10000);
			}
			else {
				arr = getNewLogs();
			}
			plotArray(arr);
		}
	}
}


function startStop() {
	halt = !halt;
	if (halt)
		document.getElementById('startStopButton').innerHTML = 'start';
	else
		document.getElementById('startStopButton').innerHTML = 'stop';
}

function clearLog() {
	clearRemoteLog();
	chartData.removeRows(0, chartData.getNumberOfRows());
	chart.draw(chartData, options);
}

function clearChart() {
	chartData.removeRows(0, chartData.getNumberOfRows());
	chart.draw(chartData, options);
}




