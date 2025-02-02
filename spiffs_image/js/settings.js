
var firstTime = true;
var firstTimeCal = true;
var firstTimeSettings = true;

var body;
var infoTbl;
var settingsTbl;
var nameTbl;
var tblBody;
var INFOTABLENAME = "infoTable";
var SETTINGSTABLENAME = "settingsTable";
var CALTABLENAME = "calTable";
var NAMETABLENAME = "nameTable";

function makeNameTable(descriptorData) {firstTimeSettings
	var colls;
	nameTbl = document.getElementById(NAMETABLENAME);// ocument.createElement("table");
	var x = nameTbl.rows.length;
	for (var r = 0; r < x; r++) {
		nameTbl.deleteRow(-1);
	}
	tblBody = document.createElement("tbody");

	var rows = descriptorData.split("\n");

	for (var i = 0; i < rows.length - 1; i++) {
		var row = document.createElement("tr");
		if (i == 0) {
			colls = rows[i].split(",");
			for (var j = 0; j < colls.length; j++) {
				var cell = document.createElement("th");
				var cellText = document.createTextNode(colls[j]);
				cell.appendChild(cellText);
				row.appendChild(cell);
			}
		}
		else {
			var cell = document.createElement("td");
			var cellText = document.createTextNode(rows[i]);
			cell.appendChild(cellText);
			row.appendChild(cell);

			cell = document.createElement("td");
			var input = document.createElement("input");
			input.setAttribute("type", "text");
			cell.appendChild(input);
			row.appendChild(cell);

			cell = document.createElement("td");
			cell.setAttribute("nameItem", i);

			var button = document.createElement("button");
			button.innerHTML = "Stel in";
			button.className = "button-3";
			cell.appendChild(button);
			row.appendChild(cell);

			cell = document.createElement("td");
			cell.setAttribute("nameItem", i);

			var button = document.createElement("button");
			button.innerHTML = "Herstel";
			button.className = "button-3";
			button.setAttribute("id", "set" + i);

			cell.appendChild(button);
			row.appendChild(cell);
		}
		tblBody.appendChild(row);
	}
	nameTbl.appendChild(tblBody);

	const cells = document.querySelectorAll("td[nameItem]");
	cells.forEach(cell => {
		cell.addEventListener('click', function () { setNameFunction(cell.closest('tr').rowIndex, cell.cellIndex) });
	});
}

function setNameFunction(row, coll) {
	console.log("Row index:" + row + " Collumn: " + coll);
	var item = nameTbl.rows[row].cells[0].innerText;

	if (coll == 3)
		sendItem("revertName");
	else {
		var value = nameTbl.rows[row].cells[1].firstChild.value;
		console.log(item + value);
		if (value != "") {
			sendItem("setName:moduleName=" + value);
		}
		makeNameTable(value);
	}
}

function makeInfoTable(descriptorData) {

	infoTbl = document.getElementById(INFOTABLENAME);// ocument.createElement("table");
	var x = infoTbl.rows.length
	for (var r = 0; r < x; r++) {
		infoTbl.deleteRow(-1);
	}
	tblBody = document.createElement("tbody");

	var rows = descriptorData.split("\n");

	for (var i = 0; i < rows.length - 1; i++) {
		var row = document.createElement("tr");
		var colls = rows[i].split(",");

		for (var j = 0; j < colls.length; j++) {
			if (i == 0)
				var cell = document.createElement("th");
			else
				var cell = document.createElement("td");

			var cellText = document.createTextNode(colls[j]);
			cell.appendChild(cellText);
			row.appendChild(cell);
		}
		tblBody.appendChild(row);
	}
	infoTbl.appendChild(tblBody);
}
function isNumeric(value) {
    return /^-?\d+$/.test(value);
}

// for calibrating and settings
function makeSettingsTable(tableName, descriptorData) {

	var colls;
	settingsTbl = document.getElementById(tableName);
	var x = settingsTbl.rows.length;
	for (var r = 0; r < x; r++) {
		settingsTbl.deleteRow(-1);
	}
	tblBody = document.createElement("tbody");

	var rows = descriptorData.split("\n");

	for (var i = 0; i < rows.length - 1; i++) {
		var row = document.createElement("tr");
		if (i == 0) {
			colls = rows[i].split(",");
			for (var j = 0; j < colls.length - 1; j++) {
				var cell = document.createElement("th");
				var cellText = document.createTextNode(colls[j]);
				cell.appendChild(cellText);
				row.appendChild(cell);
			}
		}
		else {
			
			var strs = rows[i].split(",");

			var cell = document.createElement("td");
			//var cellText = document.createTextNode(rows[i]);
			var cellText = document.createTextNode(strs[0]);
			cell.appendChild(cellText);
			row.appendChild(cell);

			cell = document.createElement("td");
			var input = document.createElement("input");

			if (isNumeric ( strs[1] ))
				input.setAttribute("type", "number");
			else
				input.setAttribute("type", "text");

			input.value = strs[1];
			cell.appendChild(input);
			row.appendChild(cell);

			cell = document.createElement("td");
			cell.setAttribute("calItem", i);

			var button = document.createElement("button");
			button.innerHTML = "Stel in";
			//	button.className = "button buttonGreen";
			button.className = "button-3";
			cell.appendChild(button);
			row.appendChild(cell);

			cell = document.createElement("td");
			cell.setAttribute("calItem", i);

		//	var button = document.createElement("button");
	//		button.innerHTML = "Herstel";
			//	button.className = "button buttonGreen";
	//		button.className = "button-3";
	//		button.setAttribute("id", "set" + i);

			cell.appendChild(button);
			row.appendChild(cell);
		}
		tblBody.appendChild(row);
	}
	settingsTbl.appendChild(tblBody);

	const cells = document.querySelectorAll("td[calItem]");
	cells.forEach(cell => {
		cell.addEventListener('click', function () { setSettingFunction(cell.closest('tr').rowIndex, cell.cellIndex,tableName) });
	});
}

function readInfo(str) {
	makeInfoTable(str);
}

function readCalInfo(str) {
	if (SIMULATE) {
		if (firstTimeCal) {
			makeSettingsTable(CALTABLENAME, str);
			firstTimeCal = false;
		}
		return;
	}
	else {
		str = getItem("getCalValues");
		makeSettingsTable(CALTABLENAME, str);
	}
}

function readSettingsInfo(str) {
	if (SIMULATE) {
		if (firstTimeSettings) {
			makeSettingsTable(SETTINGSTABLENAME, str);
			firstTimeSettings = false;
		}
		return;
	}
	else {
		str = getItem("getSettingsTable");
		makeSettingsTable(SETTINGSTABLENAME, str);
	}
}



function save() {
	getItem("saveSettings");
}

function cancel() {
	getItem("cancelSettings");
}


function setSettingFunction(row, coll,tableName) {
	var tbl = document.getElementById(tableName);

	console.log("Row index:" + row + " Collumn: " + coll);
	var item = tbl.rows[row].cells[0].innerText;

	if (coll == 3)
		sendItem("revertItem:"+ tableName + item);
	else {
		var value = tbl.rows[row].cells[1].firstChild.value;
		console.log(item + value);
		//	if (value != "") {
		sendItem("setItem:" + tableName + ":" + item + ':' + value);
		//	}
	}
}


function testInfo() {
	var str = "Meting,xActueel,xOffset,xx,\naap,2,3,4,\nnoot,5,6,7,\n,";
	readInfo(str);
	str = "Actueel,Nieuw,Stel in,Herstel,\nSensor 1\n";
	makeNameTable(str);
}

function testCal() {
	var str = "Meting,Referentie,Stel in,Herstel,\nTemperatuur\n RH\n CO2\n";
	readCalInfo(str);
}

function testSettings() {
	var str = "Item,Waarde,Stel in,Herstel,\nMiddelInterval\n Resolutie\n";
	readSettingsInfo(str);
}



function initSettings() {
	if (SIMULATE) {
		testInfo();
		testCal();
		testSettings();
	}
	else {
		readCalInfo();
		readSettingsInfo();
	//	str = getItem("getSensorName");
//		makeNameTable(str);
		setInterval(function () { settingsTimer() }, 1000);
	}
}

function tempCal() {
	var str;
	testCal();
}
var xcntr = 1;

function getInfo() {
	if (SIMULATE) {
		infoTbl.rows[1].cells[1].innerHTML = xcntr++;
		return;
	}
	var arr;
	var str;
	str = getItem("getInfoValues");
	if (firstTime) {
		makeInfoTable(str);
		firstTime = false;
	}
	else {
		var rows = str.split("\n");
		for (var i = 1; i < rows.length - 1; i++) {
			var colls = rows[i].split(",");
			for (var j = 1; j < colls.length; j++) {
				infoTbl.rows[i].cells[j].innerHTML = colls[j];
			}
		}
	}
}


function settingsTimer() {

	if (document.visibilityState == "hidden")
		return;
	getInfo();

}
