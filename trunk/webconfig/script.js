var statedata = [
{"code":"11","value":"서울특별시"},
{"code":"26","value":"부산광역시"},
{"code":"27","value":"대구광역시"},
{"code":"28","value":"인천광역시"},
{"code":"29","value":"광주광역시"},
{"code":"30","value":"대전광역시"},
{"code":"31","value":"울산광역시"},
{"code":"41","value":"경기도"},
{"code":"42","value":"강원도"},
{"code":"43","value":"충청북도"},
{"code":"44","value":"충청남도"},
{"code":"45","value":"전라북도"},
{"code":"46","value":"전라남도"},
{"code":"47","value":"경상북도"},
{"code":"48","value":"경상남도"},
{"code":"50","value":"제주특별자치도"}];

var statejson = null;
var sijson = null;
var dongjson = null;

var statecombo = byId('stateCombo');
var sicombo = byId('cityCombo');
var dongcombo = byId('dongCombo');

var stateinfo = new Object;
var siinfo = new Object;
var donginfo = new Object;

var xhrRequest = function (url, type, callback) {
	var xhr = new XMLHttpRequest();
	xhr.onload = function () {
		callback(this.responseText);
	};
	xhr.open(type, url);
	xhr.send();
};

// 도 / 광역시
function getState() {
/*
	var url = 'http://www.kma.go.kr/DFSROOT/POINT/DATA/top.json.txt';
	xhrRequest(url, 'GET', function(responseText) {
		statejson = JSON.parse(responseText);
		emptyCombo(statecombo);
		addOption(statecombo, -1, "도 선택");
		
		for(var i=0; i<statejson.length; i++) {
			addOption(statecombo, i, statejson[i].value);
			stateinfo[i] = statejson[i].code;
		}
	});
*/
		emptyCombo(statecombo);
		addOption(statecombo, -1, "도 선택");
		
		for(var i=0; i<statedata.length; i++) {
			addOption(statecombo, i, statedata[i].value);
			stateinfo[i] = statedata[i].code;
		}
};

// 시 정보
function getSi(code)
{
	var url = 'http://www.kma.go.kr/DFSROOT/POINT/DATA/mdl.'+ code + '.json.txt';
	xhrRequest(url, 'GET', function(responseText) {
		//console.log(responseText);	
		alert(responseText);
		
		sijson = JSON.parse(responseText);
		emptyCombo(sicombo);
		emptyCombo(dongcombo);
		
		addOption(sicombo, -1, "시 선택");
	
		for(var i=0; i<sijson.length; i++) {
			addOption(sicombo, i, sijson[i].value);
			siinfo[i] = sijson[i].code;
		}
	});
}

function getDong(code)
{
	var url = 'http://www.kma.go.kr/DFSROOT/POINT/DATA/leaf.' + code + '.json.txt';
	xhrRequest(url, 'GET', function(responseText) {
		//console.log(responseText);	
		alert(responseText);
		
		dongjson = JSON.parse(responseText);
		emptyCombo(dongcombo);
		addOption(dongcombo, -1, "동 선택");
	
		for(var i=0; i<dongjson.length; i++) {
			addOption(dongcombo, i, dongjson[i].value);
			
			var info = new Object();
			info.code = dongjson[i].code;
			info.x = dongjson[i].x;
			info.y = dongjson[i].y;
			donginfo[i] = info;
		}
	});
}

function getweather(x, y) 
{
	var url = 'http://www.kma.go.kr/wid/queryDFS.jsp?gridx='+x+'&gridy='+y;
	xhrRequest(url, 'GET', function(responseText) {
		weather.innerHTML = responseText;
	});
}
 


window.onload = function(){
/*
	var hw = document.getElementById('hw');
	hw.addEventListener('click', function(){
		alert('Hello world');
	})
*/
	getState();
	//alert('Hello world');
}
	
function byId(e) {return document.getElementById(e);}
 
// 도를 선택
function stateComboChange()
{
	alert(statecombo.value);
	if( statecombo.value != -1 ) 
	{
		var n = Number(statecombo.value);
		getSi(stateinfo[n]);
	}
}
 
function cityComboChange()
{
	alert(sicombo.value);
	if( sicombo.value != -1 ) 
	{
		var n = Number(sicombo.value);
		getDong(siinfo[n]);
	}
}

function dongComboChange()
{
	alert(dongcombo.value);
	if( dongcombo.value != -1 ) 
	{
		var n = Number(dongcombo.value);
		alert(n);
		//getDong(dongcomboinfo[n]);
		tgt.innerHTML = "지역 : " + donginfo[n].code + " X : " + donginfo[n].x + " Y : " + donginfo[n].y;
		
		getweather(donginfo[n].x, donginfo[n].y);
	}
}
 

/*
function stateComboChange1()
{
	var combo1 = byId('stateCombo');
	var combo2 = byId('cityCombo');
//      alert(combo1.value);

	emptyCombo(combo2);
	switch(combo1.value)
	{
		case '-1':  
					addOption(combo2, -1, '-select state first-');
					break;
		case '0':       
					addOption(combo2, 0, 'Melbourne');
					addOption(combo2, 1, 'Horsham');
					break;
		case '1':       
					addOption(combo2, 2, 'Sydney');
					addOption(combo2, 3, 'Bondi');
					break;
		case '2':       
					addOption(combo2, 4, 'Hobart');
					addOption(combo2, 5, 'Port Arthur');
					break;
	}
	cityComboChange();
}
 
function cityComboChange()
{
	var combo2, tgt;
	combo2 = byId('cityCombo');
	tgt = byId('tgt');

	tgt.innerHTML = combo2.options[combo2.options.selectedIndex].title;
}
*/

function emptyCombo(e)
{
	e.innerHTML = '';
}
 
function addOption(combo, val, txt)
{
	var option = document.createElement('option');
	option.value = val;
	option.title = txt;
	option.appendChild(document.createTextNode(txt));
	combo.appendChild(option);
}
