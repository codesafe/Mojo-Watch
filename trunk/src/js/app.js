var xmljson = require("./xml2json.js");

var usegps = -1;
var usedust = -1;
var gridx = -1;
var gridy = -1;


var xhrRequest = function (url, type, callback) {
	var xhr = new XMLHttpRequest();
	xhr.onload = function () {
		callback(this.responseText);
	};
	xhr.open(type, url);
	xhr.send();
};


function getdust(lat, lon, callback ) {
  // Construct URL
  //http://apis.skplanetx.com/weather/dust?lon=126.9658000000&lat=37.5714000000&version=1&appKey=86c4e9cf-e942-3f36-9f8d-9252e359faef
  var url = 'http://apis.skplanetx.com/weather/dust?' +
      'lon=' + lon + '&lat=' + lat + '&version=1&appKey=86c4e9cf-e942-3f36-9f8d-9252e359faef';
	var dustcode = -1;
  xhrRequest(url, 'GET', function(responseText) {
        console.log(responseText);
        var json = JSON.parse(responseText);
        var result = json.result;
        var weather = json.weather;
        if(result != null && weather != null) {
          if( result.code == 9200 ) {
            // 좋음, 보통, 약간나쁨, 나쁨, 매우나쁨
            switch(weather.dust[0].pm10.grade)
            {
              case '좋음' :
                dustcode = 0;
                break;
              case '보통' :
                dustcode = 1;
                break;
              case '약간나쁨' :
                dustcode = 2;
                break;
              case '나쁨' :
                dustcode = 3;
                break;
              case '매우나쁨' :
                dustcode = 4;
                break;
            }
          }
        }
        callback(dustcode);
      }
  );
}


function getxyPos(pos)
{
	var retpos = new Object;
	
	var v1 = pos.coords.latitude;		// 경도
	var v2 = pos.coords.longitude;		// 위도
	
	var RE = 6371.00877; // 지구 반경(km)
	var GRID = 5.0; // 격자 간격(km)
	var SLAT1 = 30.0; // 투영 위도1(degree)
	var SLAT2 = 60.0; // 투영 위도2(degree)
	var OLON = 126.0; // 기준점 경도(degree)
	var OLAT = 38.0; // 기준점 위도(degree)
	var XO = 43; // 기준점 X좌표(GRID)
	var YO = 136; // 기1준점 Y좌표(GRID)
	var DEGRAD = Math.PI / 180.0;
	// double RADDEG = 180.0 / Math.PI;

	var re = RE / GRID;
	var slat1 = SLAT1 * DEGRAD;
	var slat2 = SLAT2 * DEGRAD;
	var olon = OLON * DEGRAD;
	var olat = OLAT * DEGRAD;

	var sn = Math.tan(Math.PI * 0.25 + slat2 * 0.5) / Math.tan(Math.PI * 0.25 + slat1 * 0.5);
	sn = Math.log(Math.cos(slat1) / Math.cos(slat2)) / Math.log(sn);
	var sf = Math.tan(Math.PI * 0.25 + slat1 * 0.5);
	sf = Math.pow(sf, sn) * Math.cos(slat1) / sn;
	var ro = Math.tan(Math.PI * 0.25 + olat * 0.5);
	ro = re * sf / Math.pow(ro, sn);
	
	var ra = Math.tan(Math.PI * 0.25 + (v1) * DEGRAD * 0.5);
	ra = re * sf / Math.pow(ra, sn);
	var theta = v2 * DEGRAD - olon;
	if (theta > Math.PI)
		theta -= 2.0 * Math.PI;
	if (theta < -Math.PI)
		theta += 2.0 * Math.PI;
	theta *= sn;

	//map.put("x", Math.floor(ra * Math.sin(theta) + XO + 0.5));
	//map.put("y", Math.floor(ro - ra * Math.cos(theta) + YO + 0.5));
	
	retpos.x = Math.floor(ra * Math.sin(theta) + XO + 0.5);
	retpos.y = Math.floor(ro - ra * Math.cos(theta) + YO + 0.5)
	
	return retpos;
}

function getkorweather(lat, lon, x, y) {

	//var url = 'http://www.kma.go.kr/wid/queryDFS.jsp?gridx=58&gridy=125';
	var url = 'http://www.kma.go.kr/wid/queryDFS.jsp?gridx=' + x + 'gridy=' + y;

//	var url = 'http://map.naver.com/common2/getRegionByPosition.nhn?xPos=' +
//	'127.4833016'+'&yPos=' +'36.9719265';

	xhrRequest(url, 'GET', function(responseText) {
			console.log(responseText);

			// XML to JSon
			var retjson = xmljson.xml2json.parser(responseText);
			console.log(JSON.stringify(retjson));

			// responseText contains a JSON object with weather info
			//var json = JSON.parse(retjson);
			var json = retjson;
			
			// Temperature
			var temperature = Math.round(json.wid.body.data[0].temp);
			console.log('Temperature is ' + temperature);

			// Sky			
			//① 1 : 맑음
			//② 2 : 구름조금
			//③ 3 : 구름많음
			//④ 4 : 흐림
					
			//wfEn
			//① Clear
			//② Partly Cloudy
			//③ Mostly Cloudy
			//④ Cloudy
			//⑤ Rain
			//⑥ Snow/Rain
			//⑦ Snow			
			
			
			// 0 : 맑음 , 1 : 일부 흐림, 2 : 흐림 , 3 ; 비 , 4 : 눈
			var weathericon = 0;
			//var conditions = json.wid.body.data[0].sky;
			var conditions = json.wid.body.data[0].wfen;
			console.log('Conditions are ' + conditions);
			
			if( conditions === 'Clear' )
				weathericon = 0;
			else if( conditions === 'Partly Cloudy')
				weathericon = 1;
			else if( conditions === 'Mostly Cloudy' || conditions === 'Cloudy' )
				weathericon = 2;
			else if( conditions === 'Rain' || conditions === 'Snow/Rain' )
				weathericon = 3;
			else if( conditions === 'Snow')
				weathericon = 4;
			
			if( usedust == 1 ) {
				getdust(lat, lon, function(dustcode) {
						var dictionary = {
							'WEATHER_TEMPERATURE': temperature,
							'WEATHER_CONDITIONS' : weathericon,
							'DUST_CODE' : dustcode
						};

						// Send to Pebble
						Pebble.sendAppMessage(dictionary, 
							function(e) {
								console.log('Weather info sent to Pebble successfully!');
							},

							function(e) {
								console.log('Error sending weather info to Pebble!');
							}
						);				
					});
				
			}
			else {
					var dictionary = {
						'WEATHER_TEMPERATURE': temperature,
						'WEATHER_CONDITIONS' : weathericon,
						'DUST_CODE' : 0
					};

					// Send to Pebble
					Pebble.sendAppMessage(dictionary, 
						function(e) {
							console.log('Weather info sent to Pebble successfully!');
						},

						function(e) {
							console.log('Error sending weather info to Pebble!');
						}
					);				
			}
		}
	);
}

/*
function locationSuccess(pos) {
	// Construct URL
	// use naver map weather info
//	var url = 'http://map.naver.com/common2/getRegionByPosition.nhn?xPos=' +
//	'127.4833016'+'&yPos=' +'36.9719265';

	var url = 'http://map.naver.com/common2/getRegionByPosition.nhn?xPos=' +
	pos.coords.latitude + '&yPos=' + pos.coords.longitude;

	console.log('HTTP request GET!');

//	var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' +
//		pos.coords.latitude + '&lon=' + pos.coords.longitude + 
//		'&APPID=00ce7f5f1a49f1d44e2f34ad820a9462';

	// Send request to OpenWeatherMap
	xhrRequest(url, 'GET', function(responseText) {

			console.log(responseText);
			// responseText contains a JSON object with weather info
			var json = JSON.parse(responseText);

			// Temperature
			var temperature = Math.round(json.result.weather.temperature);
			console.log('Temperature is ' + temperature);

			// Conditions
			var conditions = json.result.weather.weatherCode;      
			console.log('Conditions are ' + conditions);
			
			var dictionary = {
				'WEATHER_TEMPERATURE': temperature,
				'CONDITIONS' : conditions
			};

			// Send to Pebble
			Pebble.sendAppMessage(dictionary, 
				function(e) {
					console.log('Weather info sent to Pebble successfully!');
				},

				function(e) {
					console.log('Error sending weather info to Pebble!');
				}
			);
		}
	);
}
*/

function locationSuccess(pos) {

	var lat = pos.coords.latitude;		// 경도
	var lon = pos.coords.longitude;		// 위도
	console.log('location : ' + lat + ' : ' + lon);
	
	if( usegps == 0 ) {
		getkorweather(lat, lon, gridx, gridy);
	}
	else {
		var newpos = getxyPos(pos);				
		getkorweather(lat, lon, newpos.x, newpos.y);				
	}
}

function locationError(err) {
	console.log('Error requesting location!');
}

function getWeather(payload) {
	
	usegps = payload.USE_POSITION;	
	usedust = payload.USE_USEDUST;
	gridx = payload.CONFIG_GRIDX;
	gridy = payload.CONFIG_GRIDY;
	
	navigator.geolocation.getCurrentPosition(
		locationSuccess,
		locationError,
		{timeout: 15000, maximumAge: 60000}
	);
/*
	if( payload.CONFIG_GRIDX == 0 || payload.CONFIG_GRIDY == 0 )
		payload.USE_POSITION = 0;
	
	if( payload.USE_POSITION == 0 ) {
		console.log('JS : use static position : ' + payload.CONFIG_GRIDX + ' : ' + payload.CONFIG_GRIDY);
		//getkorweather(62, 123);	// 성남 이매2		
		getkorweather(payload.CONFIG_GRIDX, payload.CONFIG_GRIDY);
	}
	else {
		console.log('JS : use global position');
		navigator.geolocation.getCurrentPosition( 
			locationSuccess, 
			locationError, 
			{timeout: 15000, maximumAge: 60000}
		);		
	}
*/

/*
	navigator.geolocation.getCurrentPosition(
		locationSuccess,
		locationError,
		{timeout: 15000, maximumAge: 60000}
	);
*/
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', function(e) {
		console.log('PebbleKit JS ready!');
	}
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage', function(e) {
		console.log('AppMessage received in JS! - Getting weather...');
		
		console.log('--> ' + JSON.stringify(e.payload));
		getWeather(e.payload);
	}
);

Pebble.addEventListener('showConfiguration',
	function(e) {
		console.log('showConfiguration event');
		Pebble.openURL('http://codesafe.github.io');
		//Pebble.openURL('http://asardaes.github.io/pebble-zgot/');
	}
);

Pebble.addEventListener('webviewclosed',
	function(e) {
		console.log('webviewclosed event');
		
		//Get JSON dictionary
		var configuration = JSON.parse(decodeURIComponent(e.response));
		console.log('Configuration window returned: ' + JSON.stringify(configuration));
 
		if( configuration.useposition == 0 ) {
			// 수동입력
			var pos = new Object();
			pos.coords.latitude = configuration.gridx;
			pos.coords.longitude = configuration.gridy;
			var newpos = getxyPos(pos);
			configuration.gridx = Number(newpos.x);
			configuration.gridy = Number(newpos.y);
		}
		else {
			// 위치기반
			configuration.gridx = 0;
			configuration.gridy = 0;
		}
 
		//Send to Pebble, persist there
		Pebble.sendAppMessage(
			{
				'CONFIG_USE24': configuration.use24,
				'CONFIG_POSITION': configuration.useposition,
				'CONFIG_VIBRATE': configuration.viberate,
				'CONFIG_UPDATETIME': configuration.updatetime,
				'CONFIG_GRIDX': configuration.gridx,
				'CONFIG_GRIDY': configuration.gridy,
				'CONFIG_USEDUST': configuration.usedust,
				'CONFIG_VIBRATE_BLUETOOTH': configuration.viberatebluetooth,
				'CONFIG_SCREENTYPE': configuration.blackscreen
			},
			function(e) {
				console.log('Sending settings data...');
			},
			function(e) {
				console.log('Settings feedback failed!');
			}
		);
	}
);
