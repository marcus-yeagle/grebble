var messageKeys = require('message_keys');
var Clay = require('pebble-clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig, null, {autoHandleEvents: false});
var Clay = require('pebble-clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

// Columbus, Ohio coordinates
var COLUMBUS_LAT = 39.9612;
var COLUMBUS_LON = -82.9988;

// Weather icons mapping
var WEATHER_ICONS = {
  '01d': '☀', // clear sky day
  '01n': '🌙', // clear sky night
  '02d': '⛅', // few clouds day
  '02n': '☁', // few clouds night
  '03d': '☁', // scattered clouds
  '03n': '☁',
  '04d': '☁', // broken clouds
  '04n': '☁',
  '09d': '🌧', // shower rain
  '09n': '🌧',
  '10d': '🌦', // rain
  '10n': '🌧',
  '11d': '⛈', // thunderstorm
  '11n': '⛈',
  '13d': '❄', // snow
  '13n': '❄',
  '50d': '🌫', // mist
  '50n': '🌫'
};

function fetchWeather() {
  console.log('Fetching weather for Columbus, Ohio...');
  
  // Using Open-Meteo API which doesn't require an API key and works well with Pebble
  var url = 'https://api.open-meteo.com/v1/forecast?latitude=39.9612&longitude=-82.9988&current=temperature_2m,weather_code&temperature_unit=fahrenheit&timezone=America/New_York';
  
  var xhr = new XMLHttpRequest();
  xhr.open('GET', url, true);
  xhr.timeout = 10000; // 10 second timeout
  
  xhr.onload = function() {
    if (xhr.readyState === 4) {
      if (xhr.status === 200) {
        console.log('Weather response received');
        try {
          var data = JSON.parse(xhr.responseText);
          console.log('Weather data: ' + JSON.stringify(data));
          
          var temperature = Math.round(data.current.temperature_2m);
          var weatherCode = data.current.weather_code;
          
          // Map WMO weather code to icon
          var conditionIcon = '☁';
          if (weatherCode === 0 || weatherCode === 1) {
            conditionIcon = '☀'; // Clear/Mainly clear
          } else if (weatherCode === 2 || weatherCode === 3) {
            conditionIcon = '⛅'; // Partly cloudy/Overcast
          } else if (weatherCode >= 51 && weatherCode <= 67) {
            conditionIcon = '🌧'; // Rain
          } else if (weatherCode >= 71 && weatherCode <= 77) {
            conditionIcon = '❄'; // Snow
          } else if (weatherCode >= 80 && weatherCode <= 99) {
            conditionIcon = '⛈'; // Showers/Thunderstorm
          }
          
          var message = {};
          message[messageKeys.Temperature] = temperature;
          message[messageKeys.Conditions] = conditionIcon;
          
          Pebble.sendAppMessage(message,
            function() {
              console.log('Weather sent successfully: ' + temperature + '° ' + conditionIcon);
            },
            function(error) {
              console.log('Failed to send weather: ' + JSON.stringify(error));
            }
          );
        } catch(e) {
          console.log('Error parsing weather data: ' + e.message);
        }
      } else {
        console.log('Weather request failed with status: ' + xhr.status);
      }
    }
  };
  
  xhr.onerror = function() {
    console.log('Weather request error');
  };
  
  xhr.ontimeout = function() {
    console.log('Weather request timeout');
  };
  
  xhr.send();
}

// Fetch weather on ready and every 30 minutes
Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
  fetchWeather();
  setInterval(fetchWeather, 30 * 60 * 1000);
});
