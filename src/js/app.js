/*jslint sub: true*/

// Import the Clay package
var Clay = require('pebble-clay');
// Load our Clay configuration file
var clayConfig = require('./config');
// Initialize Clay
var clay = new Clay(clayConfig);

// Message types.
var MESSAGE_TYPE_READY = 0;
var MESSAGE_TYPE_WEATHER = 1;
var MESSAGE_TYPE_SETTINGS = 2;

// Temperature units.
var TEMPERATURE_UNITS_CELSIUS = 0;
var TEMPERATURE_UNITS_FAHRENHEIT = 1;

var WEATHER_SOURCE_OPENWEATHERMAP = 1;
var WEATHER_SOURCE_YAHOO = 2;

function sendXhr(url, http_method, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(http_method, url);
  xhr.send();
}

function parseTemperature(temperature, actualTemperatureUnits, requestedTemperatureUnits) {
  temperature = parseInt(temperature, 10);
  
  if (actualTemperatureUnits == "K") {
    temperature = temperature - 273.15;  // convert from Kelvin to Celcius
    switch (requestedTemperatureUnits) {
      case TEMPERATURE_UNITS_CELSIUS:
        return  temperature;
      case TEMPERATURE_UNITS_FAHRENHEIT:
        return temperature * 9 / 5 + 32;
      default:
        console.log("PebbleKit JS received unknown temperature units: " + requestedTemperatureUnits);
      return 1 << 31;
    }
  }

  if (actualTemperatureUnits != "F") {
    console.log("Yahoo Weather API returned temperature in unknown units: " + actualTemperatureUnits);
    return 1 << 31;
  }

  switch (requestedTemperatureUnits) {
    case TEMPERATURE_UNITS_CELSIUS:
      return Math.round((temperature - 32) * 5 / 9);
    case TEMPERATURE_UNITS_FAHRENHEIT:
      return temperature;
    default:
      console.log("PebbleKit JS received unknown temperature units: " + requestedTemperatureUnits);
      return 1 << 31;
  }
}

function parseTime(dateValue, timeString) {
  var dateTime = new Date(dateValue);
  var match = timeString.match(/(\d+):(\d+)\s*(am|pm)?/);

  dateTime.setHours(parseInt(match[1], 10));
  switch (match[3].toLowerCase()) {
    case 'am':
      if (dateTime.getHours() == 12) {
        dateTime.setHours(0);
      }
      break;
    case 'pm':
      if (dateTime.getHours() != 12) {
        dateTime.setHours(dateTime.getHours() + 12);
      }
      break;
    default:
      // No correction needed.
  }

  dateTime.setMinutes(parseInt(match[2], 10));
  dateTime.setSeconds(0);

  return dateTime.valueOf();
}

function readFromLocalStorage(key, defaultValue) {
  var value = localStorage.getItem(key);
  return value === null ? defaultValue : JSON.parse(value);
}

function writeToLocalStorage(key, value) {
  localStorage.setItem(key, JSON.stringify(value));
}

function queryWeather(messageId, temperatureUnits, weatherSource, position) {

  var url;
  var myAPIKey = '31196cb8a000e808be9f27de97a6f2e1';
  if (weatherSource == WEATHER_SOURCE_YAHOO)
  {
    var query = encodeURIComponent('select astronomy, item.condition, units.temperature from weather.forecast where woeid in (select place.woeid from flickr.places where api_key="a4cd191f6a5f639df681211751f8c74e" AND lat="' + position.coords.latitude + '" AND lon="' + position.coords.longitude + '")');
    url = 'https://query.yahooapis.com/v1/public/yql?q=' + query + '&format=json';
    console.log ("requesting "+url);
  
    sendXhr(url, 'GET', function(responseText) {
      var responseJson = JSON.parse(responseText);
      var channel = responseJson.query.results.channel;
      var now = Date.now();
  
      Pebble.sendAppMessage({
        'KEY_MESSAGE_TYPE': MESSAGE_TYPE_WEATHER,
        'KEY_MESSAGE_ID': messageId,
        'KEY_CONDITION_CODE': parseInt(channel.item.condition.code, 10),
        'KEY_TEMPERATURE': parseTemperature(channel.item.condition.temp, channel.units.temperature, temperatureUnits),
        'KEY_IS_DAYLIGHT': +(parseTime(now, channel.astronomy.sunrise) <= now && now <= parseTime(now, channel.astronomy.sunset))
      });
    });
  } else {  // default to using OpenWeatherMap
     // Construct URL
    url = 'http://api.openweathermap.org/data/2.5/weather?lat=' +
        position.coords.latitude + '&lon=' + position.coords.longitude + '&appid=' + myAPIKey;
  
    // Send request to OpenWeatherMap
    sendXhr(url, 'GET', 
      function(responseText) {
        // responseText contains a JSON object with weather info
        var json = JSON.parse(responseText);
        var nowseconds = Date.now() / 1000;  // now returns in ms, but OpenWeatherMap returns sunrise and sunset in seconds.
        var sunrise = parseInt(json.sys.sunrise);
        var sunset = parseInt(json.sys.sunset);
        
        Pebble.sendAppMessage({
          'KEY_MESSAGE_TYPE': MESSAGE_TYPE_WEATHER,
          'KEY_MESSAGE_ID': messageId,
          'KEY_CONDITION_CODE': parseInt(json.weather[0].id, 10),
          'KEY_TEMPERATURE': parseTemperature(json.main.temp, "K", temperatureUnits),
          'KEY_IS_DAYLIGHT': +(sunrise <= nowseconds && nowseconds <= sunset)
         
        });
  
      }      
    );
      
  }
}

function onPositionError(error) {
  // TODO(maksym): Report geolocation request failure in settings.
  console.log("Failed to obtain geographical location.");
}

function sendWeatherRequest(messageID, temperatureUnits, weatherSource) {
  // KH:  Looked into Sean's suggestion to be notified of position updates instead of polling, but that
  //  appeared to use more battery as the updates were constantly coming in.
  navigator.geolocation.getCurrentPosition(queryWeather.bind(null, messageID, temperatureUnits, weatherSource), onPositionError, {
    timeout: 15000,
    maximumAge: 1000 * 60 * 60 * 8
  });

}

Pebble.addEventListener('ready', function(event) {
  Pebble.sendAppMessage({
    'KEY_MESSAGE_TYPE': MESSAGE_TYPE_READY
  });
});

Pebble.addEventListener('appmessage', function(event) {
  var messageType = event.payload['KEY_MESSAGE_TYPE'];
  switch (messageType) {
    case MESSAGE_TYPE_WEATHER:
      //var str = JSON.stringify(event);
      //console.log("event = "+ str);
      console.log("PebbleKit JS sending weather request with with message id of " + event.payload['KEY_MESSAGE_ID'] + " and units of " + event.payload['TEMPERATURE_UNITS'] + "and weather source of " + event.payload['WEATHER_SOURCE']);
      sendWeatherRequest(event.payload['KEY_MESSAGE_ID'], event.payload['TEMPERATURE_UNITS'], event.payload['WEATHER_SOURCE']);
      break;
    default:
      console.log("PebbleKit JS received message of unknown type: " + messageType);
      break;
  }
});

