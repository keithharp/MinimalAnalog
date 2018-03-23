module.exports = [
  {
    "type": "heading",
    "defaultValue": "Minimal Analog Configuration"
  },
 
   {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Watch Settings"
      },
      {
        "type": "radiogroup",
        "messageKey": "HAND_STYLE",
        "label": "Watch Hand Style",
        "defaultValue" : "1",
        "options": [
          { 
            "label": "Solid Pointers", 
            "value": "1"
          },
          { 
            "label": "Hollow Rectangles", 
            "value": "2"
          }
        ]
      },
      {
        "type": "radiogroup",
        "messageKey": "SHOW_SECONDS_HAND",
        "label": "When to Show Seconds Hand",
        "defaultValue": "0x00",
        "options": [
          {
            "label" : "Never",
            "value" : "0"
          },
          {
            "label" : "Always",
            "value" : "8"
          },
          {
            "label" : "Toggle On/Off When Tapped",
            "value" : "6"
          },
          {
            "label" : "Turn On Temporarily When Tapped",
            "value" : "5"
          },

        ]
      },
      {
        "type": "slider",
        "messageKey": "SECONDS_HAND_DURATION",
        "defaultValue": 2,
        "label": "How many minutes to keep showing seconds after tapping",
        "min": 1,
        "max": 60
        }
      ]
    },
    { "type": "section",
       "items": [
         {
           "type": "heading",
           "defaultValue" : "Weather Settings"
         },
         {
           "type": "radiogroup",
           "messageKey": "TEMPERATURE_UNITS",
           "label": "Temperature Units",
           "defaultValue" : "1",
           "options": [
             { 
               "label": "Fahrenheit", 
               "value": "1"
             },
             { 
               "label": "Celcius", 
               "value": "0"
             }
           ]
         },
         {
           "type": "radiogroup",
           "messageKey": "TEMPERATURE_SIZE",
           "label": "Temperature Font Size",
           "defaultValue" : "2",
           "options": [
             { 
               "label": "Small", 
               "value": "1"
             },
             { 
               "label": "Medium", 
               "value": "2"
             }
           ]
         },
         {
           "type": "radiogroup",
           "messageKey": "WEATHER_SOURCE",
           "label": "Weather Source",
           "defaultValue" : "1",
           "options": [
             { 
               "label": "OpenWeatherMap", 
               "value": "1"
             },
             { 
               "label": "Yahoo", 
               "value": "2"
             }
           ]
         },
         {
          "type": "input",
          "messageKey": "KEY_OPENWM_API",
          "label": "OpenWeatherMap API KEY",
           "defaultValue" : "31196cb8a000e808be9f27de97a6f2e1"
         }
       ]
      },
    {
        "type": "section",
        "items": [
        {
          "type": "heading",
          "defaultValue": "More Settings"
        },
  
   
  
        {
          "type": "toggle",
          "messageKey": "VIBRATE_ON_BLUETOOTH_DISCONNECT",
          "label": "Alert for Bluetooth Disconnect",
          "defaultValue": true
        }, 
        {
          "type": "slider",
          "messageKey": "SHOW_BATTERY_AT_PERCENT",
          "label": "Low Battery Threshold",
          "defaultValue": "40"
        },
        {
          "type": "toggle",
          "messageKey": "SHOW_TIMEZONE",
          "label": "Show Current TimeZone",
          "defaultValue": false
        }
  
      ]
    },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Color Settings"
      },
      {
        "type": "color",
        "messageKey": "BG_COLOR",
        "defaultValue": "0x000055",
        "label": "Background Color"
      },
      {
        "type": "color",
        "messageKey": "FG1_COLOR",
        "defaultValue": "0xAAAA55",
        "label": "Ticks, Numbers, Date, etc..."
      },      {
        "type": "color",
        "messageKey": "FG2_COLOR",
        "defaultValue": "0xFFFF55",
        "label": "Hour and Minute Hands"
      },      {
        "type": "color",
        "messageKey": "FG3_COLOR",
        "defaultValue": "0xFF0000",
        "label": "Seconds Hand"
      },
    ]
      },
      {
      "type": "section",
      "items": [
        {
        "type": "heading",
        "defaultValue": "Weather Quiet Time Settings"
      },
      {
        "type": "toggle",
        "messageKey": "WEATHER_QUIET_TIME",
        "label": "Suppress Weather Updates At These Times",
        "defaultValue": false
      },
      {
        "type": "text",
        "defaultValue": "If ALWAYS is not selected, choose the times your watch will not update the weather"
      },
      {
        "type": "slider",
        "messageKey": "WEATHER_QUIET_TIME_START",
        "defaultValue": 23,
        "label": "Weather Updates Stop (24 hr)",
        "min": 0,
        "max": 23
        },
      {
        "type": "slider",
        "messageKey": "WEATHER_QUIET_TIME_STOP",
        "defaultValue": 6,
        "label": "Weather Updates Start (24 hr)",
        "min": 0,
        "max": 23
      }
    ]
      },
    { "type": "section",
       "items": [
         {
           "type": "heading",
           "defaultValue" : "String Settings"
         },
         {
          "type": "toggle",
          "messageKey": "KEY_STRING_ON",
          "label": "Fetch String",
          "defaultValue": false
         },
         {
          "type": "input",
          "messageKey": "KEY_STRING_URL",
          "label": "URL to fetch"
         }
         ]
    },
    { "type": "section",
       "items": [
         {
           "type": "heading",
           "defaultValue" : "Ticker Settings"
         },
         {
          "type": "toggle",
          "messageKey": "KEY_TICKER_ON",
          "label": "Show Ticker",
          "defaultValue": true
         },
         {
           "type": "select",
           "messageKey": "COIN",
           "label": "Coin",
           "defaultValue" : "1",
           "options": [
             { 
               "label": "bitcoin", 
               "value": "1"
             },
             { 
               "label": "ethereum", 
               "value": "2"
             },
             { 
               "label": "ripple", 
               "value": "3"
             },
             { 
               "label": "lightcoin", 
               "value": "4"
             },
             { 
               "label": "bitcoin-cash", 
               "value": "5"
             }, 
             { 
               "label": "ethereum-classic", 
               "value": "6"
             }
           ]
         },
         {
           "type": "select",
           "messageKey": "CURRENCY",
           "label": "Currency",
           "defaultValue" : "1",
           "options": [
             { 
               "label": "usd", 
               "value": "1"
             },
             { 
               "label": "aud", 
               "value": "2"
             },
             { 
               "label": "can", 
               "value": "3"
             },
             { 
               "label": "nzd", 
               "value": "4"
             },
             { 
               "label": "eur", 
               "value": "5"
             },
             { 
               "label": "pnd", 
               "value": "6"
             },
           ]
         },   
       ]
      },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
