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


function onPositionSuccess(messageId, temperatureUnits, position) {
  var query = encodeURIComponent('select astronomy, item.condition, units.temperature from weather.forecast where woeid in (select place.woeid from flickr.places where api_key="a4cd191f6a5f639df681211751f8c74e" AND lat="' + position.coords.latitude + '" AND lon="' + position.coords.longitude + '")');
  var url = 'https://query.yahooapis.com/v1/public/yql?q=' + query + '&format=json';

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
}

function onPositionError(error) {
  // TODO(maksym): Report geolocation request failure in settings.
  console.log("Failed to obtain geographical location.");
}

function sendWeatherRequest(messageId, temperatureUnits) {
  // TODO(maksym): Subscribe to position updates,
  //               see http://developer.getpebble.com/guides/js-apps/pebblekit-js/js-capabilities/
  navigator.geolocation.getCurrentPosition(onPositionSuccess.bind(null, messageId, temperatureUnits), onPositionError, {
    timeout: 15000,
    maximumAge: 60000
  });
}

Pebble.addEventListener('ready', function(event) {
  Pebble.sendAppMessage({
    'KEY_MESSAGE_TYPE': MESSAGE_TYPE_READY
  });
});

Pebble.addEventListener('appmessage', function(event) {
  //ar str = JSON.stringify(event);
  //console.log("Message dump"+ str);
  var messageType = event.payload['KEY_MESSAGE_TYPE'];
  switch (messageType) {
    case MESSAGE_TYPE_WEATHER:
      console.log("PebbleKit JS sending weather request with units of " + event.payload['TEMPERATURE_UNITS']);
      sendWeatherRequest(event.payload['KEY_MESSAGE_ID'], event.payload['TEMPERATURE_UNITS']);
      break;
    default:
      console.log("PebbleKit JS received message of unknown type: " + messageType);
      break;
  }
});


//Pebble.addEventListener('showConfigurationXXX', function(event) {

  // IMPORTANT: Keep default values in sync with purist_window.c.
  //            Do not use booleans, they are distorted when sending the message from Android.
//  var settings = {
//    'MESSAGE_KEY_SHOW_SECONDS_HAND': readFromLocalStorage('MESSAGE_KEY_SHOW_SECONDS_HAND', 0),
//    'MESSAGE_KEY_TEMPERATURE_UNITS': readFromLocalStorage('MESSAGE_KEY_TEMPERATURE_UNITS', TEMPERATURE_UNITS_FAHRENHEIT),
//    'MESSAGE_KEY_VIBRATE_ON_BLUETOOTH_DISCONNECT': readFromLocalStorage('MESSAGE_KEY_VIBRATE_ON_BLUETOOTH_DISCONNECT', 1),
////    'MESSAGE_KEY_SHOW_BATTERY_AT_PERCENT': readFromLocalStorage('MESSAGE_KEY_SHOW_BATTERY_AT_PERCENT', 40),
//    'MESSAGE_KEY_HAND_STYLE': readFromLocalStorage('MESSAGE_KEY_HAND_STYLE', 1),
//    'MESSAGE_KEY_TEMPERATURE_SIZE': readFromLocalStorage('MESSAGE_KEY_TEMPERATURE_SIZE', 2)
//  };
//  Pebble.openURL('http://www.reliantwebsolutions.com/pebble-config-test2.html?settings=' + encodeURIComponent(JSON.stringify(settings)));
//});

//Pebble.addEventListener('webviewclosedXX', function(event) {
//  if (event.response === '') {
//    return;
//  }

//  var settings = JSON.parse(decodeURIComponent(event.response));
//  for (var key in settings) {
//    if (settings.hasOwnProperty(key)) {
//      writeToLocalStorage(key, settings[key]);
//    }
//  }
//
//  settings['KEY_MESSAGE_TYPE'] = MESSAGE_TYPE_SETTINGS;
//  Pebble.sendAppMessage(settings);
//});
