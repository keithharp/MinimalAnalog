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
        "type": "toggle",
        "messageKey": "SHOW_SECONDS_HAND",
        "label": "Enable Seconds",
        "defaultValue": false
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
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
