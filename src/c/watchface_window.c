/*
 * Minimal Analog 2 - A watchface for Pebble, Pebble Time, and Pebble Round
 *
 *  Note that according to https://forums.pebble.com/t/how-to-recover-from-app-msg-busy-after-bluetooth-reconnects/22948
 *   the problem I am seeing after bluetooth disconnect where the weather does not get updated is a pebble firmware bug.
 *   workaround is to go into an app and come back to the watchface.
 *
 * Version 1.6 adds
 *   - initial cryptocurrency ticker support. Whole dollar value only.
 *
 * Version 1.5 adds
 *   - code cleanup on font to weather condition mapping to make it easier to read - DONE
 *   - add DIORITE support - DONE
 *   - add EMERY support - Need to test, but now adapts to different size screens.
 *
 * Version 1.4 adds
 *   - provide ability to choose OpenWeatherMap or Yahoo  (yahoo had stopped working for me) - DONE
 *
 * Version 1.3 adds
 *   - provide a workaround for a pebble bug that hoses communication after a bluetooth reconnect.  Restart the watchface if this error occurs. --DONE
 *
 * Version 1.2 adds
 *   - user options to turn on second hand for a defined period or until tapped again by tapping. - DONE
 *   - pay attention to bluetooth status to avoid pinging for weather if not connected to save battery - DONE
 *   - keep cached location and weather and don't overwrite with the sync logo -- DONE by moving sync logo to BT location
 *   - refactor settings code to avoid repeating the same action if multiple settings require an update to a watchface element. -- DONE
 *
 * Version 1.1 adds
 *    - user option for restricting weather updates to certain hours, default 7am to 11pm - DONE
 *
 * Version 1.0
 *
 *  Keith Harp -- keithharp@bellsouth.net
 *
 * Based on the original design and work by Sean Ullyatt of Reliant Web Solutions
 * After he decided he could no longer work on it, he graciously allowed anyone to pick
 * it up and take it on.
 *
 * Minimal Analog - A watchface for Pebble, Pebble Steel, Pebble Time, and Pebble Time Steel
 * Version 2.0
 *
 * Items that were on Sean's TODO list were:
  * TODO:
 *   - Add user option to show battery indicator at a certain charge percent, default 40% - DONE
 *   - Add options for setting colors, foreground, background, weather, bluetooth, etc. - DONE
 *   - Add user option to select the style of hand - traditional or space - DONE
 *   - Add user option for restricting weather updates to certain hours, default 7am to 11pm - DONE
 *   - Add user option for user to select date format, default “%a %d"
 *   - Add weather forecast in bluetooth area on double wrist-flick
 *   - Improve battery life by avoiding as many location checks - TESTED but REJECTED
 *
 * In addition, I have the following thoughts
 * TODO:
 *   - Replace web based settings (no longer available) with Clay based ones - DONE
 *   - Allow a larger font choice for the temperature - DONE
 *   - Use Quiet Time as the time to avoid weather updates rather than (or in addition to) fixed times - Not possible with current APIs
 *
 * TODO:
 *   - Allow font selection for coin ticker
 *
 * Sean Ullyatt - sullyatt@gmail.com
 * Reliant Web Solutions, LLC (c) 2015 - All Right Reserved
 *

 */
#include "watchface_window.h"
#include "pebble_patch.h"
#include <limits.h>

typedef struct {
  int message_type;

  union {
    // Weather message.
    struct {
      int message_id;
      int condition_code;
      int temperature;
      bool is_daylight;
    };
    
    // Ticker message.
    struct {
       int message_id1;
       char* ticker; 
    };

    // Settings message.
    struct {
      int seconds_hand_mode;
      int temperature_units;
      bool vibrate_on_bluetooth_disconnect;
      int show_battery_at_percent;
      int hand_style;
      int bg_color;
      int fg1_color;
      int fg2_color;
      int fg3_color;
      int temperature_font_size;
      int ticker_font_size;
      bool weather_quiet_time;
      int weather_quiet_time_start;
      int weather_quiet_time_stop;
      int seconds_hand_duration;
      int weather_source;
      bool show_timezone;
      int coin;
      int currency;
      bool show_ticker;
    };
  };
} Message;

// Keys used in all messages.
#define KEY_MESSAGE_TYPE 0

// Keys used in weather message.
#define KEY_MESSAGE_ID 1
#define KEY_CONDITION_CODE 2
#define KEY_TEMPERATURE 3
#define KEY_IS_DAYLIGHT 4

// Keys used in ticker message
#define KEY_TICKER 21
#define KEY_MESSAGE_ID1 23

// Keys used in settings message.
#define MESSAGE_KEY_SHOW_SECONDS_HAND 5
#define MESSAGE_KEY_TEMPERATURE_UNITS 6
#define MESSAGE_KEY_VIBRATE_ON_BLUETOOTH_DISCONNECT 7
#define MESSAGE_KEY_SHOW_BATTERY_AT_PERCENT 8
#define MESSAGE_KEY_HAND_STYLE 9
#define MESSAGE_KEY_BG_COLOR 10
#define MESSAGE_KEY_FG1_COLOR 11
#define MESSAGE_KEY_FG2_COLOR 12
#define MESSAGE_KEY_FG3_COLOR 13
#define MESSAGE_KEY_TEMPERATURE_SIZE 14
#define MESSAGE_KEY_WEATHER_QUIET_TIME 15
#define MESSAGE_KEY_WEATHER_QUIET_TIME_START 16
#define MESSAGE_KEY_WEATHER_QUIET_TIME_STOP 17
#define MESSAGE_KEY_SECONDS_HAND_DURATION 18
#define MESSAGE_KEY_WEATHER_SOURCE 19
#define MESSAGE_KEY_SHOW_TIMEZONE 20
#define MESSAGE_KEY_TICKER_ON 26

// Message types.
#define MESSAGE_TYPE_READY 0
#define MESSAGE_TYPE_WEATHER 1
#define MESSAGE_TYPE_SETTINGS 2
#define MESSAGE_TYPE_TICKER 3

// Temperature units.
#define TEMPERATURE_UNITS_CELSIUS 0
#define TEMPERATURE_UNITS_FAHRENHEIT 1

// Ticker settings
#define MESSAGE_KEY_COIN 24
#define MESSAGE_KEY_CURRENCY 25

// Weather sources  -- Yahoo stopped working for me as of 8/23/2016.  Appears to be a problem with converting lat/long to WOEID
#define WEATHER_SOURCE_OPENWEATHERMAP 1
#define WEATHER_SOURCE_YAHOO 2

// state of how to handle seconds hand  -- 0x08 mask means show seconds hand   0x04 means we need to be registerd for the tap sensor
#define SHOW_SECONDS_HAND(x)  (x & 0x8)
#define TAP_SENSOR_NEEDED(x)  (x & 0x4)
#define SECONDS_HAND_OFF 0x0                        // binary 0000 or 0
#define SECONDS_HAND_ON 0x8                         // binary 1000 or 8
#define SECONDS_HAND_FOR_FIXED_DURATION_OFF 0x5     // binary 0101 or 5
#define SECONDS_HAND_FOR_FIXED_DURATION_ON 0xd      // binary 1101 or 13
#define SECONDS_HAND_TOGGLE_TAP_OFF 0x6             // binary 0110 or 6
#define SECONDS_HAND_TOGGLE_TAP_ON 0xe              // binary 1110 or 15


// Condition codes.
// Reference: https://developer.yahoo.com/weather/documentation.html#codes
#define CONDITION_CODE_REFRESH -1
#define CONDITION_CODE_TORNADO 0
#define CONDITION_CODE_TROPICAL_STORM 1
#define CONDITION_CODE_HURRICANE 2
#define CONDITION_CODE_SEVERE_THUNDERSTORMS 3
#define CONDITION_CODE_THUNDERSTORMS 4
#define CONDITION_CODE_MIXED_RAIN_AND_SNOW 5
#define CONDITION_CODE_MIXED_RAIN_AND_SLEET 6
#define CONDITION_CODE_MIXED_SNOW_AND_SLEET 7
#define CONDITION_CODE_FREEZING_DRIZZLE 8
#define CONDITION_CODE_DRIZZLE 9
#define CONDITION_CODE_FREEZING_RAIN 10
#define CONDITION_CODE_SHOWERS 11
#define CONDITION_CODE_SHOWERS_ALIAS 12
#define CONDITION_CODE_SNOW_FLURRIES 13
#define CONDITION_CODE_LIGHT_SNOW_SHOWERS 14
#define CONDITION_CODE_BLOWING_SNOW 15
#define CONDITION_CODE_SNOW 16
#define CONDITION_CODE_HAIL 17
#define CONDITION_CODE_SLEET 18
#define CONDITION_CODE_DUST 19
#define CONDITION_CODE_FOGGY 20
#define CONDITION_CODE_HAZE 21
#define CONDITION_CODE_SMOKY 22
#define CONDITION_CODE_BLUSTERY 23
#define CONDITION_CODE_WINDY 24
#define CONDITION_CODE_COLD 25
#define CONDITION_CODE_CLOUDY 26
#define CONDITION_CODE_MOSTLY_CLOUDY_NIGHT 27
#define CONDITION_CODE_MOSTLY_CLOUDY_DAY 28
#define CONDITION_CODE_PARTLY_CLOUDY_NIGHT 29
#define CONDITION_CODE_PARTLY_CLOUDY_DAY 30
#define CONDITION_CODE_CLEAR_NIGHT 31
#define CONDITION_CODE_SUNNY 32
#define CONDITION_CODE_FAIR_NIGHT 33
#define CONDITION_CODE_FAIR_DAY 34
#define CONDITION_CODE_MIXED_RAIN_AND_HAIL 35
#define CONDITION_CODE_HOT 36
#define CONDITION_CODE_ISOLATED_THUNDERSTORMS 37
#define CONDITION_CODE_SCATTERED_THUNDERSTORMS 38
#define CONDITION_CODE_SCATTERED_THUNDERSTORMS_ALIAS 39
#define CONDITION_CODE_SCATTERED_SHOWERS 40
#define CONDITION_CODE_HEAVY_SNOW 41
#define CONDITION_CODE_SCATTERED_SNOW_SHOWERS 42
#define CONDITION_CODE_HEAVY_SNOW_ALIAS 43
#define CONDITION_CODE_PARTLY_CLOUDY 44
#define CONDITION_CODE_THUNDERSHOWERS 45
#define CONDITION_CODE_SNOW_SHOWERS 46
#define CONDITION_CODE_ISOLATED_THUNDERSHOWERS 47

// OpenWeatherMap condition codes:  http://openweathermap.org/weather-conditions
#define OWM_CONDITION_CODE_THUNDERSTORM_MIN 200
#define OWM_CONDITION_CODE_THUNDERSTORM_MAX 299
#define OWM_CONDITION_CODE_RAIN_MIN 300
#define OWM_CONDITION_CODE_RAIN_MAX 599
#define OWM_CONDITION_CODE_SNOW_MIN 600
#define OWM_CONDITION_CODE_LIGHT_RAIN_AND_SNOW 615
#define OWM_CONDITION_CODE_RAIN_AND_SNOW 616
#define OWM_CONDITION_CODE_SNOW_MAX 699
#define OWM_CONDITION_CODE_ATMOSPHERE_MIN 700
#define OWM_CONDITION_CODE_ATMOSPHERE_MAX 799
#define OWM_CONDITION_CODE_CLEAR 800
#define OWM_CONDITION_CODE_PARTLY_CLOUDY_MIN 801
#define OWM_CONDITION_CODE_PARTLY_CLOUDY_MAX 803
#define OWM_CONDITION_CODE_OVERCAST_CLOUDS 804
#define OWM_CONDITION_CODE_TORNADO 900
#define OWM_CONDITION_CODE_TROPICAL_STORM 901
#define OWM_CONDITION_CODE_HURRICANE 902
#define OWM_CONDITION_CODE_COLD 903
#define OWM_CONDITION_CODE_HOT 904
#define OWM_CONDITION_CODE_WINDY 905
#define OWM_CONDITION_CODE_HAIL 906

// Symbols available in the myicons-webfont.ttf
#define ICON_REFRESH "f"
#define ICON_TORNADO "A"
#define ICON_HURRICANE "B"
#define ICON_THUNDERSTORM "C"
#define ICON_FREEZING_RAIN "D"
#define ICON_RAIN "E"
#define ICON_SNOW "F"
#define ICON_FOG "G"
#define ICON_WINDY "H"
#define ICON_COLD "I"
#define ICON_CLOUDY "J"
#define ICON_PARTLY_CLOUDY_DAY "L"
#define ICON_PARTLY_CLOUDY_NIGHT "K"
#define ICON_CLEAR_DAY "N"
#define ICON_CLEAR_NIGHT "M"
#define ICON_HOT "O"
#define ICON_UNKNOWN "d"
#define ICON_NONE ""
#define ICON_RESTART "h"
#define ICON_CHARGING "s"
#define ICON_PLUGGED "t"
#define ICON_BLUETOOTH_DISCONNECT "b"
 
int const MIN_WEATHER_UPDATE_INTERVAL_MS = 10 * 1000;
int const MAX_WEATHER_UPDATE_INTERVAL_MS = 30 * 60 * 1000;

typedef struct {
  int seconds_hand_mode;
  int seconds_hand_duration;
  AppTimer *seconds_duration_timer; 
  time_t most_recent_tap;
  
  int temperature_units;
  bool vibrate_on_bluetooth_disconnect;
  bool bluetooth_connected;
  int show_battery_at_percent;
  int hand_style;
  int temperature_font_size;
  int ticker_coin;
  int ticker_currency;
  int weather_source;
  int ticker_font_size;
  
  bool show_timezone;
  bool show_ticker;
  
  GFont font_hours;
  GFont font_date;
  GFont font_ticker;
  GFont font_ticker_small;
  GFont font_temperature;
  GFont font_temperature_small;
  // GFont font_temperature_medium;   The medium temperature font just reuses the font_date so no need to reload it.
  GFont font_condition;
  GFont font_battery;
  GFont font_bluetooth;

  GColor color_background;
  GColor color_foreground_1; // ticks, numbers, date, weather, bluetooth
  GColor color_foreground_2; // hour and minute hands
  GColor color_foreground_3; // second hand
  
  // Hex values for the colors above.
  int bg_color;
  int fg1_color;
  int fg2_color;
  int fg3_color;

  bool weather_quiet_time;
  int weather_quiet_time_start;
  int weather_quiet_time_stop;
  
  Layer *background_layer;

  TextLayer *date_text_layer;
  char date_text[sizeof("Jan 31")];

  TextLayer *ticker_text_layer;
  char ticker_text[7];

  TextLayer *temperature_text_layer;
  char temperature_text[sizeof("-999°")];

  TextLayer *condition_text_layer;
  char condition_text[sizeof("C")];

  Layer *battery_layer;
  TextLayer *battery_text_layer;
  char battery_text[sizeof("C")];

  TextLayer *bluetooth_text_layer;
  char bluetooth_text[sizeof("b")];
  
  TextLayer *timezone_text_layer;
  char timezone_text[sizeof("NAEDT")];  //  Longest one I could find was 5 chars. 

  GPath *hour_hand_path;
  GPath *minute_hand_path;

  Layer *hands_layer;
  Layer *second_hand_layer;

  AppTimer *weather_update_timer;
  int weather_update_backoff_interval;
  int expected_weather_message_id;
    
  AppTimer *ticker_update_timer;
  int ticker_update_backoff_interval;
  int expected_ticker_message_id;
} WatchfaceWindow;

static Window *g_watchface_window = NULL;

static void force_immediate_time_update(Window* watchface_window, bool bUpdateTime, bool bUpdateTickTimerService, bool bUpdateDurationTimer);

static void update_background(Layer *layer, GContext *ctx) {
  WatchfaceWindow *this = window_get_user_data(layer_get_window(layer));
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);

#ifdef PBL_COLOR
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_width(ctx, 3);
#endif

  graphics_context_set_fill_color(ctx, this->color_background);
  graphics_context_set_stroke_color(ctx, this->color_foreground_1);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);

  int16_t ray_length = 111;
  int ray_angles[8] = { 5, 10, 20, 25, 35, 40, 50, 55 };

  for (int i = 0; i < 8; ++i) {
    GPoint ray_endpoint = {
      .x = (int16_t)(sin_lookup(TRIG_MAX_ANGLE * ray_angles[i] / 60) * (int32_t)ray_length / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(TRIG_MAX_ANGLE * ray_angles[i] / 60) * (int32_t)ray_length / TRIG_MAX_RATIO) + center.y,
    };

    graphics_draw_line(ctx, ray_endpoint, center);
  }

  graphics_context_set_fill_color(ctx, this->color_background);
  graphics_context_set_stroke_color(ctx, this->color_background);
#if defined(PBL_ROUND)
  // Draw background color circle to cover hour rays
  graphics_fill_circle(ctx, center, bounds.size.h/2 - 10);
#else
  // Draw background color rectangle to cover hour rays
  graphics_fill_rect(ctx, GRect(10, 10, bounds.size.w - 20, bounds.size.h - 20), 0, GCornerNone);
#endif
  
  // Draw hours
  graphics_context_set_text_color(ctx, this->color_foreground_1);
  graphics_draw_text(ctx, "12", this->font_hours, GRect((bounds.size.w / 2) - 15, -5, 30, 24), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  graphics_draw_text(ctx, "3", this->font_hours, GRect(bounds.size.w - 31, (bounds.size.h / 2) - 15, 30, 24), GTextOverflowModeWordWrap, GTextAlignmentRight, NULL);
  graphics_draw_text(ctx, "6", this->font_hours, GRect((bounds.size.w / 2) - 15, bounds.size.h - 26, 30, 24), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  graphics_draw_text(ctx, "9", this->font_hours, GRect(1, (bounds.size.h / 2) - 15, 30, 24), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
}

static void update_ticker(Window *watchface_window, char* ticker) {
  WatchfaceWindow *this = window_get_user_data(watchface_window);

  GRect ticker_text_layer_frame = layer_get_frame(text_layer_get_layer(this->ticker_text_layer));
  char const *format;
  format = "$%s";
  snprintf(this->ticker_text, sizeof(this->ticker_text), format, ticker);
  text_layer_set_text(this->ticker_text_layer, this->ticker_text);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Updating Ticker to : %s", this->ticker_text);
  layer_set_frame(text_layer_get_layer(this->ticker_text_layer), ticker_text_layer_frame);
  text_layer_set_text_color(this->ticker_text_layer, this->color_foreground_1);
}

static void update_date(Window *watchface_window) {
  WatchfaceWindow *this = window_get_user_data(watchface_window);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
 
  strftime(this->date_text, sizeof(this->date_text), "%a %d", t);
  text_layer_set_text(this->date_text_layer, this->date_text);
  text_layer_set_text_color(this->date_text_layer, this->color_foreground_1);
}

static void update_timezone(Window *watchface_window, char* tz) {
  WatchfaceWindow *this = window_get_user_data(watchface_window);
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "update timezone to %s", tz);
  
  strncpy(this->timezone_text, tz,  sizeof(this->timezone_text));
  text_layer_set_text(this->timezone_text_layer, this->timezone_text);
  text_layer_set_text_color(this->timezone_text_layer, this->color_foreground_1);
}

static void update_temperature(Window *watchface_window, int temperature) {
  WatchfaceWindow *this = window_get_user_data(watchface_window);

  char const *format;
  GRect temperature_text_layer_frame = layer_get_frame(text_layer_get_layer(this->temperature_text_layer));
  if (temperature < 0) {
    format = temperature >= -999 ? "%d°" : "";
  } else {
    format = temperature <= 999 ? "%d°" : "";
  }

  snprintf(this->temperature_text, sizeof(this->temperature_text), format, temperature);
  text_layer_set_text(this->temperature_text_layer, this->temperature_text);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Updating Temp to : %s", this->temperature_text);
  layer_set_frame(text_layer_get_layer(this->temperature_text_layer), temperature_text_layer_frame);
  text_layer_set_text_color(this->temperature_text_layer, this->color_foreground_1);
}

static void condition_code_to_icon_yahoo(Window *watchface_window, int condition_code, bool is_daylight) {
  WatchfaceWindow *this = window_get_user_data(watchface_window);

  char* icon = NULL;
  
  // Set symbol using icon font
  switch (condition_code) {
    case CONDITION_CODE_REFRESH:
      icon = ICON_REFRESH;
      break;
    case CONDITION_CODE_TORNADO:
      icon = ICON_TORNADO;
      break;
    case CONDITION_CODE_TROPICAL_STORM:
    case CONDITION_CODE_HURRICANE:
      icon = ICON_HURRICANE;
      break;
    case CONDITION_CODE_THUNDERSTORMS:
    case CONDITION_CODE_SEVERE_THUNDERSTORMS:
    case CONDITION_CODE_ISOLATED_THUNDERSTORMS:
    case CONDITION_CODE_SCATTERED_THUNDERSTORMS:
    case CONDITION_CODE_SCATTERED_THUNDERSTORMS_ALIAS:
    case CONDITION_CODE_THUNDERSHOWERS:
    case CONDITION_CODE_ISOLATED_THUNDERSHOWERS:
      icon = ICON_THUNDERSTORM;
      break;
    case CONDITION_CODE_FREEZING_RAIN:
    case CONDITION_CODE_FREEZING_DRIZZLE:
    case CONDITION_CODE_MIXED_RAIN_AND_SNOW:
    case CONDITION_CODE_MIXED_RAIN_AND_SLEET:
    case CONDITION_CODE_HAIL:
    case CONDITION_CODE_SLEET:
      icon = ICON_FREEZING_RAIN;
      break;
    case CONDITION_CODE_SHOWERS:
    case CONDITION_CODE_SHOWERS_ALIAS:
    case CONDITION_CODE_SCATTERED_SHOWERS:
    case CONDITION_CODE_DRIZZLE:
    case CONDITION_CODE_MIXED_RAIN_AND_HAIL:
      icon = ICON_RAIN;
      break;
    case CONDITION_CODE_SNOW:
    case CONDITION_CODE_SNOW_FLURRIES:
    case CONDITION_CODE_MIXED_SNOW_AND_SLEET:
    case CONDITION_CODE_LIGHT_SNOW_SHOWERS:
    case CONDITION_CODE_BLOWING_SNOW:
    case CONDITION_CODE_HEAVY_SNOW:
    case CONDITION_CODE_SCATTERED_SNOW_SHOWERS:
    case CONDITION_CODE_HEAVY_SNOW_ALIAS:
    case CONDITION_CODE_SNOW_SHOWERS:
      icon = ICON_SNOW;
      break;
    case CONDITION_CODE_DUST:
    case CONDITION_CODE_FOGGY:
    case CONDITION_CODE_HAZE:
    case CONDITION_CODE_SMOKY:
      icon = ICON_FOG;
      break;
    case CONDITION_CODE_BLUSTERY:
    case CONDITION_CODE_WINDY:
      icon = ICON_WINDY;
      break;
    case CONDITION_CODE_COLD:
      icon = ICON_COLD;
      break;
    case CONDITION_CODE_CLOUDY:
    case CONDITION_CODE_MOSTLY_CLOUDY_DAY:
    case CONDITION_CODE_MOSTLY_CLOUDY_NIGHT:
      icon = ICON_CLOUDY;
      break;
    case CONDITION_CODE_PARTLY_CLOUDY:
    case CONDITION_CODE_PARTLY_CLOUDY_DAY:
    case CONDITION_CODE_PARTLY_CLOUDY_NIGHT:
      icon = (is_daylight ? ICON_PARTLY_CLOUDY_DAY : ICON_PARTLY_CLOUDY_NIGHT);
      break;
    case CONDITION_CODE_SUNNY:
    case CONDITION_CODE_FAIR_DAY:
    case CONDITION_CODE_CLEAR_NIGHT:
    case CONDITION_CODE_FAIR_NIGHT:
      icon = (is_daylight ? ICON_CLEAR_DAY : ICON_CLEAR_NIGHT );
      break;
    case CONDITION_CODE_HOT:
      icon = ICON_HOT;
      break;
    default:
      icon = ICON_UNKNOWN;
      break;
  }
  strncpy(this->condition_text, icon, sizeof(this->condition_text));

}
 

// OpenWeatherMap condition codes:  http://openweathermap.org/weather-conditions

static void condition_code_to_icon_openweathermap(Window *watchface_window, int condition_code, bool is_daylight) {
  WatchfaceWindow *this = window_get_user_data(watchface_window);
  
  char* icon = NULL;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "condition code = %d %d", condition_code, is_daylight);
  
  if (condition_code >= OWM_CONDITION_CODE_THUNDERSTORM_MIN && condition_code <= OWM_CONDITION_CODE_THUNDERSTORM_MAX) // thunderstorms
    icon = ICON_THUNDERSTORM;
  else if (condition_code >= OWM_CONDITION_CODE_RAIN_MIN && condition_code <= OWM_CONDITION_CODE_RAIN_MAX) // rain or drizzle
    icon = ICON_RAIN;
  else if (condition_code >= OWM_CONDITION_CODE_SNOW_MIN && condition_code <= OWM_CONDITION_CODE_SNOW_MAX) // snow
    if (condition_code == OWM_CONDITION_CODE_LIGHT_RAIN_AND_SNOW || condition_code == OWM_CONDITION_CODE_RAIN_AND_SNOW) // mixed snow and rain
      icon = ICON_FREEZING_RAIN;
    else
      icon = ICON_SNOW;
  else if (condition_code >= OWM_CONDITION_CODE_ATMOSPHERE_MIN && condition_code <= OWM_CONDITION_CODE_ATMOSPHERE_MAX) // mist/snoke/haze, etc...
    icon = ICON_FOG;
  else if (condition_code >= OWM_CONDITION_CODE_PARTLY_CLOUDY_MIN && condition_code <= OWM_CONDITION_CODE_PARTLY_CLOUDY_MAX) // partly cloudy
    icon = (is_daylight ? ICON_PARTLY_CLOUDY_DAY : ICON_PARTLY_CLOUDY_NIGHT);
  else {
 
      switch (condition_code) {
        case CONDITION_CODE_REFRESH:
          icon = ICON_REFRESH;
          break;
        case OWM_CONDITION_CODE_CLEAR: 
          icon = (is_daylight ? ICON_CLEAR_DAY : ICON_CLEAR_NIGHT);
          break;      
        case OWM_CONDITION_CODE_OVERCAST_CLOUDS: 
          icon = ICON_CLOUDY;
          break; 
        case OWM_CONDITION_CODE_TORNADO: 
          icon = ICON_TORNADO;
          break;
        case OWM_CONDITION_CODE_TROPICAL_STORM: 
        case OWM_CONDITION_CODE_HURRICANE:  
          icon = ICON_HURRICANE;
          break;
        case OWM_CONDITION_CODE_COLD: 
          icon = ICON_COLD;
          break; 
        case OWM_CONDITION_CODE_HOT:
          icon = ICON_HOT;
          break; 
        case OWM_CONDITION_CODE_WINDY:  
          icon = ICON_WINDY;
          break;
        case OWM_CONDITION_CODE_HAIL: 
          icon = ICON_FREEZING_RAIN;
          break;
   
        default:
          icon = ICON_UNKNOWN;
          break;
      }
  }
  strncpy(this->condition_text, icon, sizeof(this->condition_text));
}

static void condition_code_to_icon(Window *watchface_window, int condition_code, bool is_daylight) {
  WatchfaceWindow *this = window_get_user_data(watchface_window);

  if (this->weather_source == WEATHER_SOURCE_YAHOO)
    condition_code_to_icon_yahoo(watchface_window, condition_code, is_daylight);
  else
    condition_code_to_icon_openweathermap(watchface_window, condition_code, is_daylight);
}

static void mark_as_syncing(Window* watchface_window, bool syncing) {
#if 0
  update_condition(watchface_window, CONDITION_CODE_REFRESH, false);
  update_temperature(watchface_window, INT_MIN);  
#else
  WatchfaceWindow *this = window_get_user_data(watchface_window);

  strncpy(this->bluetooth_text, syncing ? ICON_REFRESH : ICON_NONE , sizeof(this->bluetooth_text));
#endif
}

static void update_condition(Window *watchface_window, int condition_code, bool is_daylight) {
  WatchfaceWindow *this = window_get_user_data(watchface_window);
  
  mark_as_syncing(watchface_window, false);
  condition_code_to_icon(watchface_window, condition_code, is_daylight);
  text_layer_set_text(this->condition_text_layer, this->condition_text);
  text_layer_set_text_color(this->condition_text_layer, this->color_foreground_1);
}

static void log_reason(char* info, AppMessageResult reason) {
  char reason_string[1000];
  reason_string[0] = 0;
  if (reason == APP_MSG_OK) {
    strcat(reason_string, "OK");
  } else {
    if (reason & APP_MSG_OK) {
      strcat(reason_string, "OK.");
    }
    if (reason & APP_MSG_SEND_TIMEOUT) {
      strcat(reason_string, "Send Timeout.");
    }
    if (reason & APP_MSG_SEND_REJECTED) {
      strcat(reason_string, "Rejected.");
    }
    if (reason & APP_MSG_NOT_CONNECTED) {
      strcat(reason_string, "Not Connected.");
    }
    if (reason & APP_MSG_APP_NOT_RUNNING) {
      strcat(reason_string, "App Not Running.");
    }
    if (reason & APP_MSG_INVALID_ARGS) {
      strcat(reason_string, "Invalid Args.");
    }
    if (reason & APP_MSG_BUSY) {
      strcat(reason_string, "Busy.");
    }
    if (reason & APP_MSG_BUFFER_OVERFLOW) {
      strcat(reason_string, "Buffer Overflow.");
    }
    if (reason & APP_MSG_ALREADY_RELEASED) {
      strcat(reason_string, "Already Released.");
    } 
    if (reason & APP_MSG_OUT_OF_MEMORY) {
      strcat(reason_string, "Out of Memory.");
    }
    if (reason & APP_MSG_INTERNAL_ERROR) {
      strcat(reason_string, "Internal Error.");
    } 
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "%s %x %s", info, reason, reason_string); 
}

static void cancel_ticker_update_timer(WatchfaceWindow* this) {
  
   // In case there is already a timer, turn it off
  if (this->ticker_update_timer) {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "getting ready to close weather_timer");
    app_timer_cancel(this->ticker_update_timer);
    this->ticker_update_timer = NULL;
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "done with resetting timer");
  }
}

static void cancel_weather_update_timer(WatchfaceWindow* this) {
  
   // In case there is already a timer, turn it off
  if (this->weather_update_timer) {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "getting ready to close weather_timer");
    app_timer_cancel(this->weather_update_timer);
    this->weather_update_timer = NULL;
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "done with resetting timer");
  }
}

// Drastic measures...  If we get stuck to where we can no longer get messages, then restart the watchface
static void restart_watchface(void* watchface_window) {
  WatchfaceWindow *this = window_get_user_data(watchface_window);
  time_t now = time(NULL);

  strncpy(this->bluetooth_text, ICON_RESTART, sizeof(this->bluetooth_text));
  
  // normally it will restart on it's own, but occasionally, that doesn't seem to happen.  Set timer
  // to wake up after X seconds in that case.
  wakeup_schedule(now + 5, 0, false); 
  APP_LOG(APP_LOG_LEVEL_ERROR, "restarting watchface to recover from Pebble communications bug");
  window_stack_pop_all(false);
}


static void send_weather_request(void *watchface_window) {
  WatchfaceWindow *this = window_get_user_data(watchface_window);
  int result = 0;
    
  if (!this->bluetooth_connected)
    return; // short circuit and stop looking for weather if bluetooth is currently disconnected

  // Set up a timer in case we do not get an update
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "About to set weather update timer");
  this->weather_update_timer = app_timer_register(this->weather_update_backoff_interval, send_weather_request, watchface_window);
  this->weather_update_backoff_interval *= 2;
  if (this->weather_update_backoff_interval > MAX_WEATHER_UPDATE_INTERVAL_MS) {
    this->weather_update_backoff_interval = MAX_WEATHER_UPDATE_INTERVAL_MS;
  }

  DictionaryIterator *iterator;
  if ((result = app_message_outbox_begin(&iterator)) != APP_MSG_OK) {
    log_reason("unable to begin outbox", result); 
    //  There is a bug documented in  https://forums.pebble.com/t/how-to-recover-from-app-msg-busy-after-bluetooth-reconnects/22948 
    //    where we get a BUSY that we never recover from after bluetooth reconnect.   So, let's reboot the app to recover.
    if (result == APP_MSG_BUSY) restart_watchface(watchface_window);
  }  else {
    dict_write_int32(iterator, KEY_MESSAGE_TYPE, MESSAGE_TYPE_WEATHER);
    dict_write_int32(iterator, KEY_MESSAGE_ID, ++this->expected_weather_message_id);
    dict_write_int32(iterator, MESSAGE_KEY_WEATHER_SOURCE, this->weather_source);
    dict_write_int32(iterator, MESSAGE_KEY_TEMPERATURE_UNITS, this->temperature_units);
    dict_write_int32(iterator, MESSAGE_KEY_TICKER_ON, this->show_ticker);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "In C function send_weather_request temperature units : %i and source = %d", this->temperature_units, this->weather_source);
    if (this->show_ticker) {
      dict_write_int32(iterator, KEY_MESSAGE_ID1, ++this->expected_ticker_message_id);
      dict_write_int32(iterator, MESSAGE_KEY_COIN, this->ticker_coin);
      dict_write_int32(iterator, MESSAGE_KEY_CURRENCY, this->ticker_currency);
    }
    if ((result = app_message_outbox_send()) != APP_MSG_OK) 
      log_reason("unable to send outbox", result);
  }    
  //mark_as_syncing(watchface_window, true);

}

static void update_battery_state(Layer *layer, GContext *ctx) {
  WatchfaceWindow *this = window_get_user_data(layer_get_window(layer));

  BatteryChargeState battery_state = battery_state_service_peek();
  char* icon = NULL;
  
  if (battery_state.is_charging) { // Set symbol using icon font
    icon = ICON_CHARGING;
  } else if (battery_state.is_plugged) {
    icon = ICON_PLUGGED;
  } else {
    icon = ICON_NONE;
  }
  strncpy(this->battery_text, icon, sizeof(this->battery_text));

  text_layer_set_text(this->battery_text_layer, this->battery_text);

  if (battery_state.charge_percent > this->show_battery_at_percent && !battery_state.is_charging && !battery_state.is_plugged) {
    return; // Battery does not need shown
  }

  graphics_context_set_stroke_color(ctx, this->color_foreground_1);
  graphics_context_set_fill_color(ctx, this->color_foreground_1);
  graphics_draw_rect(ctx, GRect(0, 0, 14, 8));
  graphics_fill_rect(ctx, GRect(2, 2, (battery_state.charge_percent / 10), 4), 0, GCornerNone);
}


static void do_async_weather_update(Window* watchface_window) {
   WatchfaceWindow *this = window_get_user_data(watchface_window);
  
   cancel_weather_update_timer(this);
   this->weather_update_backoff_interval = MIN_WEATHER_UPDATE_INTERVAL_MS;
   send_weather_request(watchface_window);  
}

static void update_bluetooth(Window *watchface_window, bool bluetooth_connected) {
  WatchfaceWindow *this = window_get_user_data(watchface_window);

  strncpy(this->bluetooth_text, (bluetooth_connected ? ICON_NONE : ICON_BLUETOOTH_DISCONNECT), sizeof(this->bluetooth_text));
 
  this->bluetooth_connected = bluetooth_connected;
  
  text_layer_set_text(this->bluetooth_text_layer, this->bluetooth_text);
  
  if (bluetooth_connected)  {// if we just got reconnected, then update the weather
    do_async_weather_update(watchface_window);
  }
}

static void update_hands(Layer *layer, GContext *ctx) {
  WatchfaceWindow *this = window_get_user_data(layer_get_window(layer));

  GRect bounds = layer_get_bounds(this->hands_layer);
  GPoint center = grect_center_point(&bounds);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

#ifdef PBL_COLOR
  graphics_context_set_antialiased(ctx, true);
#endif

  // destroy old hands
  gpath_destroy(this->hour_hand_path);
  gpath_destroy(this->minute_hand_path);

  if (this->hand_style == 1) // Traditional
  {
    GPathInfo hour_hand_points = { 3, (GPoint []){ {7, 0}, {0, -50}, {-7, 0} } };
    GPathInfo minute_hand_points = { 3, (GPoint []) { {7, 0}, {0, -75}, {-7, 0} } };

    // create new hands
    this->hour_hand_path = gpath_create(&hour_hand_points);
    this->minute_hand_path = gpath_create(&minute_hand_points);
    gpath_move_to(this->hour_hand_path, center);
    gpath_move_to(this->minute_hand_path, center);

    // hour hand
    graphics_context_set_stroke_color(ctx, this->color_background);
    graphics_context_set_fill_color(ctx, this->color_foreground_2);
    gpath_rotate_to(this->hour_hand_path, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
    gpath_draw_filled(ctx, this->hour_hand_path);
    gpath_draw_outline(ctx, this->hour_hand_path);

    // draw circle above hour hand, under minute hand
    graphics_context_set_stroke_color(ctx, this->color_background);
    graphics_draw_circle(ctx, center, 6);

    // minute hand
    graphics_context_set_stroke_color(ctx, this->color_background);
    graphics_context_set_fill_color(ctx, this->color_foreground_2);
    gpath_rotate_to(this->minute_hand_path, TRIG_MAX_ANGLE * t->tm_min / 60);
    gpath_draw_filled(ctx, this->minute_hand_path);
    gpath_draw_outline(ctx, this->minute_hand_path);

    // dot in the middle
    graphics_context_set_fill_color(ctx, this->color_foreground_2);
    graphics_fill_circle(ctx, center, 5);
    graphics_context_set_stroke_color(ctx, this->color_background);
    graphics_draw_circle(ctx, center, 3);
    graphics_context_set_fill_color(ctx, this->color_background);
    graphics_fill_circle(ctx, center, 1);
  }
  else if (this->hand_style == 2) // Space
  {
    GPathInfo hour_hand_points = { 4, (GPoint []) { {4, 0}, {4, -45}, {-4, -45}, {-4, 0} } };
    GPathInfo minute_hand_points = { 4, (GPoint []) { {4, 0}, {4, -70}, {-4, -70}, {-4, 0} } };

    // create new hands
    this->hour_hand_path = gpath_create(&hour_hand_points);
    this->minute_hand_path = gpath_create(&minute_hand_points);
    gpath_move_to(this->hour_hand_path, center);
    gpath_move_to(this->minute_hand_path, center);

    // hour hand
    graphics_context_set_stroke_color(ctx, this->color_foreground_2);
    graphics_context_set_fill_color(ctx, this->color_background);
    gpath_rotate_to(this->hour_hand_path, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
    gpath_draw_filled(ctx, this->hour_hand_path);
    gpath_draw_outline(ctx, this->hour_hand_path);

    // minute hand
    graphics_context_set_stroke_color(ctx, this->color_foreground_2);
    graphics_context_set_fill_color(ctx, this->color_background);
    gpath_rotate_to(this->minute_hand_path, TRIG_MAX_ANGLE * t->tm_min / 60);
    gpath_draw_filled(ctx, this->minute_hand_path);
    gpath_draw_outline(ctx, this->minute_hand_path);

    // disc in the middle
    graphics_context_set_fill_color(ctx, this->color_background);
    graphics_fill_circle(ctx, center, 7);
    graphics_context_set_stroke_color(ctx, this->color_foreground_2);
    graphics_draw_circle(ctx, center, 8);
  }
}



static void update_seconds(Layer *layer, GContext *ctx) {
  WatchfaceWindow *this = window_get_user_data(layer_get_window(layer));
  
#ifdef PBL_COLOR
  graphics_context_set_antialiased(ctx, true);
#endif
  
  if (SHOW_SECONDS_HAND(this->seconds_hand_mode)) {
    GRect bounds = layer_get_bounds(layer);
    GPoint center = grect_center_point(&bounds);
    int16_t second_hand_length = bounds.size.w / 2;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
    GPoint second_hand = {
      .x = (int16_t)(sin_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.x,
      .y = (int16_t)(-cos_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.y,
    };

    graphics_context_set_stroke_color(ctx, this->color_foreground_3);
    graphics_draw_line(ctx, second_hand, center);
  }
}


static bool inQuietTime(WatchfaceWindow *this, int hour) {
  
  bool quiet = true;
  if (!this->weather_quiet_time)
    quiet = false;
  else if (this->weather_quiet_time_start == this->weather_quiet_time_stop)  // if both values are the same, then never update the weather
    quiet = true;
  else if (this->weather_quiet_time_start < this->weather_quiet_time_stop ) // this would be like start at 1am, and stop at 6am
    quiet = (hour >= this->weather_quiet_time_start) && (hour < this->weather_quiet_time_stop);
  else  // this would be like start at 11pm and stop at 6am
    quiet = (hour >= this->weather_quiet_time_start) || (hour < this->weather_quiet_time_stop);
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "checking for inQuietTime (%d %d %d) ==  %d", this->weather_quiet_time, this->weather_quiet_time_start, this->weather_quiet_time_stop, quiet);     
  return quiet;  
}

static void update_time(Window *watchface_window, struct tm *local_time) {
  WatchfaceWindow *this = window_get_user_data(watchface_window);

  //if (SHOW_SECONDS_HAND(this->seconds_hand_mode)) {
    layer_mark_dirty(this->second_hand_layer);
  //}

  if (this->show_timezone && (strcmp(local_time->tm_zone, this->timezone_text) != 0)) {
    update_timezone(watchface_window, local_time->tm_zone);
  } 

  if (local_time->tm_sec == 0) { // Top of minute
    layer_mark_dirty(this->hands_layer);

    if (local_time->tm_min == 0 && local_time->tm_hour == 0) { // Midnight
        update_date(watchface_window);
    }
    
#if 0
    // this is for testing updates... comment out for actual releases  ... provides update every 5 minutes
   APP_LOG(APP_LOG_LEVEL_DEBUG, "countdown to weather update %d", local_time->tm_min % 5);

   if ((local_time->tm_min % 5 == 0) && !inQuietTime(this, local_time->tm_hour)) { // Top and bottom of hour
      APP_LOG(APP_LOG_LEVEL_DEBUG, "doing call for weather in debug mode");
#else
    if ((local_time->tm_min == 30 || local_time->tm_min == 0) && !inQuietTime(this, local_time->tm_hour)) { // Top and bottom of hour
#endif
      do_async_weather_update(watchface_window);

    }
  }
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  update_time(g_watchface_window, tick_time);
}

static void handle_battery_state(BatteryChargeState battery_state) {
  WatchfaceWindow *this = window_get_user_data(g_watchface_window);
  layer_mark_dirty(this->battery_layer);
}

static void handle_bluetooth(bool bluetooth_connected) {
  WatchfaceWindow *this = window_get_user_data(g_watchface_window);

  update_bluetooth(g_watchface_window, bluetooth_connected);

  if (!bluetooth_connected && this->vibrate_on_bluetooth_disconnect) {
    vibes_double_pulse();
  }
}

static void watchface_tick_timer_service_subscribe(Window *watchface_window) {
  WatchfaceWindow *this = window_get_user_data(watchface_window);

  tick_timer_service_unsubscribe();

  //APP_LOG (APP_LOG_LEVEL_DEBUG, "updating tick timer service %02x %d", this->seconds_hand_mode, SHOW_SECONDS_HAND(this->seconds_hand_mode));
  if (SHOW_SECONDS_HAND(this->seconds_hand_mode)) {
    tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
  }
  else {
    tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
  }
}

static Layer *battery_layer_create(int midX, int midY) {
  Layer *battery_layer = layer_create(GRect(midX - 7, 35, 14, 8));
  layer_set_update_proc(battery_layer, update_battery_state);
  return battery_layer;
}

GFont get_weather_font(WatchfaceWindow *this) {
  switch (this->temperature_font_size) {
    case 1: // small font;
      if (this->font_temperature_small == NULL)
         this->font_temperature_small =  fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_EPITET_REGULAR_12)); 
      return this->font_temperature_small;
    break;
    default: // no font picked.  Default to medium font, but log an error first
      APP_LOG(APP_LOG_LEVEL_ERROR, "trying to set weather font, but value out of range %d", this->temperature_font_size);      
    case 2: // medium font  (which is the same font used for date)
       return this->font_date;
    break;
  }
}

  
GFont get_ticker_font(WatchfaceWindow *this) {
  switch (this->ticker_font_size) {
    case 1: // small font;
      if (this->font_ticker_small == NULL)
         this->font_ticker_small =  fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_EPITET_REGULAR_12)); 
      return this->font_ticker_small;
    break;
    default: // no font picked.  Default to medium font, but log an error first
      APP_LOG(APP_LOG_LEVEL_ERROR, "trying to set ticker font, but value out of range %d", this->ticker_font_size);      
    case 2: // medium font  (which is the same font used for date)
       return this->font_date;
    break;
  }
}

  
  
static TextLayer *watchface_text_layer_create(GRect layer_bounds, GFont layer_font, GColor layer_color) {
  TextLayer *new_text_layer = text_layer_create(layer_bounds);
  text_layer_set_font(new_text_layer, layer_font);
  text_layer_set_background_color(new_text_layer, GColorClear);
  text_layer_set_text_color(new_text_layer, layer_color);
  text_layer_set_text_alignment(new_text_layer, GTextAlignmentCenter);
  return new_text_layer;
}




static void turn_off_seconds_after_timer(void* watch_window) {
  WatchfaceWindow* this = (WatchfaceWindow*)watch_window;
  
  this->seconds_duration_timer = NULL;
  if (this->seconds_hand_mode == SECONDS_HAND_FOR_FIXED_DURATION_ON) {
    this->seconds_hand_mode = SECONDS_HAND_FOR_FIXED_DURATION_OFF;
    force_immediate_time_update(g_watchface_window, true, true, false);
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "received request to turn off seconds based on timer but currently in wrong mode %d", this->seconds_hand_mode);
  }
}



static void register_seconds_duration_timer(Window* watchface_window) {
  WatchfaceWindow *this = window_get_user_data(watchface_window);
  int duration = this->seconds_hand_duration * 60000;
  
  if (this->seconds_duration_timer)  { // if it already exists, reschedule it.
      app_timer_reschedule(this->seconds_duration_timer, duration);  
   } else {
      this->seconds_duration_timer = app_timer_register(duration, turn_off_seconds_after_timer, this);
  }
}


static void force_immediate_time_update(Window* watchface_window, bool bUpdateTime, bool bUpdateTickTimerService, bool bUpdateDurationTimer) {
   
  if (bUpdateTime) {
    update_time(watchface_window, local_time_peek());
  }
  if (bUpdateTickTimerService) {
    watchface_tick_timer_service_subscribe(watchface_window);
  }
  if (bUpdateDurationTimer) {
    register_seconds_duration_timer(watchface_window);
  }
}


static void tap_received(AccelAxisType axis, int32_t direction) {
  // A tap event occured
  WatchfaceWindow *this = window_get_user_data(g_watchface_window);
  time_t now = time(NULL);
  bool double_tap = false;
  
  bool bUpdateTime = false;
  bool bUpdateTickTimerService = false;
  bool bUpdateDurationTimer = false;
  
  double_tap = (now - this->most_recent_tap < 2);
  this->most_recent_tap = now;
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "tap %d %lx", axis, direction);
  // ignore double, triple taps, based on if time is less than a second apart
  if (double_tap) {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "ignored tap");
  } else {
  
    switch (this->seconds_hand_mode) {
      case SECONDS_HAND_FOR_FIXED_DURATION_OFF:  // not already showing... start showing for fixed duration
        this->seconds_hand_mode = SECONDS_HAND_FOR_FIXED_DURATION_ON;
        bUpdateTime = true;
        bUpdateTickTimerService = true;
        bUpdateDurationTimer = true;
      break;
      case SECONDS_HAND_FOR_FIXED_DURATION_ON:  // already showing for fixed duration... Restart the timer
        bUpdateDurationTimer = true;
      break;
      case SECONDS_HAND_TOGGLE_TAP_OFF:        // currently not showing seconds hand.  Start showing it.
        this->seconds_hand_mode = SECONDS_HAND_TOGGLE_TAP_ON;
        bUpdateTime = true;
        bUpdateTickTimerService = true;
      break;
      case SECONDS_HAND_TOGGLE_TAP_ON:        // currently showning seconds hand.  Stop showing it.
        this->seconds_hand_mode = SECONDS_HAND_TOGGLE_TAP_OFF;
        bUpdateTime = true;
        bUpdateTickTimerService = true;
      break;
      default:
        APP_LOG(APP_LOG_LEVEL_ERROR, "received tap when state does not expect one.  Current mode = %02x", this->seconds_hand_mode);
      break;
    }
    
    force_immediate_time_update(g_watchface_window, bUpdateTime, bUpdateTickTimerService, bUpdateDurationTimer);
  }
  
}


static void tap_service_subscribe(WatchfaceWindow* this) {
    accel_tap_service_subscribe(tap_received);
}


static void watchface_window_load(Window *watchface_window) {
  WatchfaceWindow *this = window_get_user_data(watchface_window);

  Layer *root_layer = window_get_root_layer(watchface_window);
  GRect bounds = layer_get_bounds(root_layer);
  int midX = bounds.size.w / 2;
  int midY = bounds.size.h / 2;

  this->font_hours = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_EPITET_REGULAR_24));
  this->font_date = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_EPITET_REGULAR_15));
  this->font_ticker = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_EPITET_REGULAR_12));
  //this->font_ticker = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ICONS_12));
  //this->font_ticker = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  this->font_temperature_small = NULL;
  this->font_temperature = get_weather_font(this);
  this->font_ticker_small = NULL;
  this->font_condition = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ICONS_24));
  this->font_battery = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ICONS_12));
  this->font_bluetooth = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ICONS_36));

  this->color_background = GColorFromHEX(this->bg_color); 
  this->color_foreground_1 = GColorFromHEX(this->fg1_color);
  this->color_foreground_2 = GColorFromHEX(this->fg2_color);
  this->color_foreground_3 = GColorFromHEX(this->fg3_color);

  this->background_layer = layer_create(bounds);
  layer_set_update_proc(this->background_layer, update_background);
  layer_add_child(root_layer, this->background_layer);

  this->date_text_layer = watchface_text_layer_create(GRect(midX - 30, midY*2- 50, 60, 20), this->font_date, this->color_foreground_1);
  layer_add_child(root_layer, text_layer_get_layer(this->date_text_layer));
    
  this->ticker_text_layer = watchface_text_layer_create(GRect(midX +12, midY - 21, 50, 30), this->font_ticker, this->color_foreground_1);
  layer_add_child(root_layer, text_layer_get_layer(this->ticker_text_layer));

  this->temperature_text_layer = watchface_text_layer_create(GRect(midX/4, midY + 6, 44, 20), this->font_temperature, this->color_foreground_1);
  layer_add_child(root_layer, text_layer_get_layer(this->temperature_text_layer));

  this->condition_text_layer = watchface_text_layer_create(GRect(midX/4, midY - 21, 44, 30), this->font_condition, this->color_foreground_1);
  layer_add_child(root_layer, text_layer_get_layer(this->condition_text_layer));

  this->battery_layer = battery_layer_create(midX, midY);  // midX - 7, 35, 14, 8
  layer_add_child(root_layer, this->battery_layer);

  this->battery_text_layer = watchface_text_layer_create(GRect(midX +9, 34, 18, 14), this->font_battery, this->color_foreground_1);
  layer_add_child(root_layer, text_layer_get_layer(this->battery_text_layer));

  this->bluetooth_text_layer = watchface_text_layer_create(GRect(midX + 10, midY - 22, 50, 50), this->font_bluetooth, this->color_foreground_1);
  layer_add_child(root_layer, text_layer_get_layer(this->bluetooth_text_layer));
  
  this->timezone_text_layer = watchface_text_layer_create(GRect(midX - 30, 47, 60, 20), this->font_date, this->color_foreground_1);
  layer_add_child(root_layer, text_layer_get_layer(this->timezone_text_layer));
  
  this->bluetooth_connected = false;

  this->hands_layer = layer_create(bounds);
  layer_set_update_proc(this->hands_layer, update_hands);
  layer_add_child(root_layer, this->hands_layer);
  
  this->second_hand_layer = layer_create(bounds);
  layer_set_update_proc(this->second_hand_layer, update_seconds);
  layer_add_child(root_layer, this->second_hand_layer);
  
  if (TAP_SENSOR_NEEDED(this->seconds_hand_mode)) {
    tap_service_subscribe(this);
  }
}
  


static void watchface_window_appear(Window *watchface_window) {
  ConnectionHandlers handlers = {handle_bluetooth, NULL};

  update_time(watchface_window, local_time_peek());
  watchface_tick_timer_service_subscribe(watchface_window);

  battery_state_service_subscribe(handle_battery_state);

  update_bluetooth(watchface_window, connection_service_peek_pebble_app_connection());
  connection_service_subscribe(handlers);
}

static void watchface_window_disappear(Window *watchface_window) {
  WatchfaceWindow *this = window_get_user_data(watchface_window);
  
  connection_service_unsubscribe();
  battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();
  if (TAP_SENSOR_NEEDED(this->seconds_hand_mode)) {
    accel_tap_service_unsubscribe();
  }
}

static void watchface_window_unload(Window *watchface_window) {
  WatchfaceWindow *this = window_get_user_data(watchface_window);

  if (this->seconds_duration_timer) {
    app_timer_cancel(this->seconds_duration_timer);
    this->seconds_duration_timer = NULL;
  }
  
  cancel_weather_update_timer(this);
    cancel_ticker_update_timer(this);

  gpath_destroy(this->hour_hand_path);
  gpath_destroy(this->minute_hand_path);

  layer_destroy(this->second_hand_layer);
  this->second_hand_layer = NULL;

  layer_destroy(this->hands_layer);
  this->hands_layer = NULL;

  text_layer_destroy(this->bluetooth_text_layer);
  this->bluetooth_text_layer = NULL;

  text_layer_destroy(this->battery_text_layer);
  this->battery_text_layer = NULL;

  layer_destroy(this->battery_layer);
  this->battery_layer = NULL;

  text_layer_destroy(this->condition_text_layer);
  this->condition_text_layer = NULL;

  text_layer_destroy(this->temperature_text_layer);
  this->temperature_text_layer = NULL;

  text_layer_destroy(this->ticker_text_layer);
  this->ticker_text_layer = NULL;
    
  text_layer_destroy(this->date_text_layer);
  this->date_text_layer = NULL;
  
  text_layer_destroy(this->timezone_text_layer);
  this->timezone_text_layer = NULL;

  layer_destroy(this->background_layer);
  this->background_layer = NULL;

  fonts_unload_custom_font(this->font_bluetooth);
  this->font_bluetooth = NULL;

  if (this->font_temperature_small != NULL) {
    fonts_unload_custom_font(this->font_temperature_small);
    this->font_temperature_small = NULL;
  }
  this->font_temperature = NULL;
  
  fonts_unload_custom_font(this->font_battery);
  this->font_battery = NULL;

  fonts_unload_custom_font(this->font_condition);
  this->font_condition = NULL;

  fonts_unload_custom_font(this->font_date);
  this->font_date = NULL;

  fonts_unload_custom_font(this->font_ticker);
  this->font_ticker = NULL;
    
  fonts_unload_custom_font(this->font_hours);
  this->font_hours = NULL;
}

static void ready_received(void *watchface_window) {
  do_async_weather_update(watchface_window);;
  update_date(watchface_window);
}

static void weather_received(void *watchface_window, Message const *message) {
  WatchfaceWindow *this = window_get_user_data(watchface_window);

  if (message->message_id == this->expected_weather_message_id) {
    cancel_weather_update_timer(this);

    update_condition(watchface_window, message->condition_code, message->is_daylight);
    update_temperature(watchface_window, message->temperature);
  }
}

static void ticker_received(void *watchface_window, Message const *message) {
  WatchfaceWindow *this = window_get_user_data(watchface_window);

  if (message->message_id1 == this->expected_ticker_message_id) {
    cancel_ticker_update_timer(this);

    update_ticker(watchface_window, message->ticker);
  }
}


static void settings_received(void *watchface_window, Message const *message) {
  WatchfaceWindow *this = window_get_user_data(watchface_window);
  
  bool bUpdateTime = false;
  bool bUpdateTickTimer = true;
  bool bUpdateTapSensor = true;
  bool bUpdateWeather = false;
  
  if (this->seconds_hand_duration != message->seconds_hand_duration) {
    this->seconds_hand_duration = message->seconds_hand_duration;
    persist_write_int(MESSAGE_KEY_SECONDS_HAND_DURATION, this->seconds_hand_duration);
  }

  if (this->seconds_hand_mode != message->seconds_hand_mode) {
    bUpdateTime = true;
    bUpdateTickTimer =  (SHOW_SECONDS_HAND(message->seconds_hand_mode) != SHOW_SECONDS_HAND(this->seconds_hand_mode));
    if (TAP_SENSOR_NEEDED(message->seconds_hand_mode) != TAP_SENSOR_NEEDED(this->seconds_hand_mode)) {
      bUpdateTapSensor = true;
      if (TAP_SENSOR_NEEDED(message->seconds_hand_mode))  // if it used to need the tap sensor, but no longer does, then unsubscribe
          accel_tap_service_unsubscribe();
    }
    this->seconds_hand_mode = message->seconds_hand_mode;
    persist_write_int(MESSAGE_KEY_SHOW_SECONDS_HAND, this->seconds_hand_mode);
  }

  if (this->temperature_units != message->temperature_units) {
    this->temperature_units = message->temperature_units;
    persist_write_int(MESSAGE_KEY_TEMPERATURE_UNITS, this->temperature_units);
    bUpdateWeather = true;
  }
  
  if (this->ticker_coin != message->coin) {
      this->ticker_coin = message->coin;
      persist_write_int(MESSAGE_KEY_COIN, this->ticker_coin);
      bUpdateWeather = true;
  }

  if (this->ticker_currency != message->currency) {
      this->ticker_currency = message->currency;
      persist_write_int(MESSAGE_KEY_CURRENCY, this->ticker_currency);
      bUpdateWeather = true;
  }
    
  if (this->vibrate_on_bluetooth_disconnect != message->vibrate_on_bluetooth_disconnect) {
    this->vibrate_on_bluetooth_disconnect = message->vibrate_on_bluetooth_disconnect;
    persist_write_bool(MESSAGE_KEY_VIBRATE_ON_BLUETOOTH_DISCONNECT, this->vibrate_on_bluetooth_disconnect);
  }

  if (this->show_timezone != message->show_timezone) {
    this->show_timezone = message->show_timezone;
    persist_write_bool(MESSAGE_KEY_SHOW_TIMEZONE, this->show_timezone);
    update_timezone(watchface_window, "");  // just make it null.   It will update on the next tick if we want to see the timezone
  }

  if (this->show_ticker != message->show_ticker) {
    this->show_ticker = message->show_ticker;
    persist_write_bool(MESSAGE_KEY_TICKER_ON, this->show_ticker);
    update_ticker(watchface_window, "");
  }
  
  if (this->show_battery_at_percent != message->show_battery_at_percent) {
    this->show_battery_at_percent = message->show_battery_at_percent;
    persist_write_int(MESSAGE_KEY_SHOW_BATTERY_AT_PERCENT, this->show_battery_at_percent);
    layer_mark_dirty(this->battery_layer);
  }

  if (this->hand_style != message->hand_style) {
    this->hand_style = message->hand_style;
    persist_write_int(MESSAGE_KEY_HAND_STYLE, this->hand_style);
    bUpdateTime = true;
  }
  
  if (this->temperature_font_size != message->temperature_font_size) {
    this->temperature_font_size = message->temperature_font_size;
    persist_write_int(MESSAGE_KEY_TEMPERATURE_SIZE, this->temperature_font_size);
    this->font_temperature = get_weather_font(this);
    text_layer_set_font(this->temperature_text_layer, this->font_temperature);
  }
  
  if (this->bg_color != message->bg_color) {
    this->bg_color = message->bg_color;
    persist_write_int(MESSAGE_KEY_BG_COLOR, this->bg_color);
    this->color_background = GColorFromHEX(message->bg_color);
    layer_mark_dirty(this->background_layer);
  }
  
  if (this->fg1_color != message->fg1_color) {
    this->fg1_color = message->fg1_color;
    persist_write_int(MESSAGE_KEY_FG1_COLOR, this->fg1_color);
    this->color_foreground_1 = GColorFromHEX(message->fg1_color);
    layer_mark_dirty(this->background_layer);
    bUpdateWeather = true;
    update_date(watchface_window);
  }
    
  if (this->fg2_color != message->fg2_color) {
    this->fg2_color = message->fg2_color;
    persist_write_int(MESSAGE_KEY_FG2_COLOR, this->fg2_color);
    this->color_foreground_2 = GColorFromHEX(message->fg2_color);
  }
  
  if (this->fg3_color != message->fg3_color) {
    this->fg3_color = message->fg3_color;
    persist_write_int(MESSAGE_KEY_FG3_COLOR, this->fg3_color);
    this->color_foreground_3 = GColorFromHEX(message->fg3_color);
  }
  
  if (this->weather_quiet_time != message->weather_quiet_time) {
    this->weather_quiet_time = message->weather_quiet_time;
    persist_write_bool(MESSAGE_KEY_WEATHER_QUIET_TIME, this->weather_quiet_time);
  }

  if (this->weather_quiet_time_start != message->weather_quiet_time_start) {
    this->weather_quiet_time_start = message->weather_quiet_time_start;
    persist_write_int(MESSAGE_KEY_WEATHER_QUIET_TIME_START, this->weather_quiet_time_start);
  }

  if (this->weather_quiet_time_stop != message->weather_quiet_time_stop) {
    this->weather_quiet_time_stop = message->weather_quiet_time_stop;
    persist_write_int(MESSAGE_KEY_WEATHER_QUIET_TIME_STOP, this->weather_quiet_time_stop);
  }
  
  if (this->weather_source != message->weather_source) {
    this->weather_source = message->weather_source;
    persist_write_int(MESSAGE_KEY_WEATHER_SOURCE, this->weather_source);
    bUpdateWeather = true;
  }
  
  if (bUpdateWeather) 
    do_async_weather_update(watchface_window);
  
  force_immediate_time_update(watchface_window, bUpdateTime, bUpdateTickTimer, false);
  
  if (bUpdateTapSensor && TAP_SENSOR_NEEDED(this->seconds_hand_mode)) {
    tap_service_subscribe(this);
  }
 
}

static void set_clay_message(Message *message)
// In the original HTML based settings code, we set our settings messsage with a message type.
//  However, so far, I have not found a unique way to flag CLAY based settings, so anytime we get a Clay based setting,
//  we flag this message as having come from Clay.
{
  message->message_type = MESSAGE_TYPE_SETTINGS;  
}

  
static void outbox_sent(DictionaryIterator *iterator, void *context) {
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "outbox sent");
}




static void outbox_failed(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  log_reason("outbox failed to send", reason);
}


static void inbox_received(DictionaryIterator *iterator, void *watchface_window) {
  Message message;
  memset(&message, 0xff, sizeof(message));

  for (Tuple *tuple = dict_read_first(iterator); tuple != NULL; tuple = dict_read_next(iterator)) {

    switch(tuple->key) {
      case KEY_MESSAGE_TYPE:
        message.message_type = tuple->value->int32;
        break;
      case KEY_MESSAGE_ID:
        message.message_id = tuple->value->int32;
        break;
      case KEY_MESSAGE_ID1:
        message.message_id1 = tuple->value->int32;
        break;
      case KEY_CONDITION_CODE:
        message.condition_code = tuple->value->int32;
        break;
      case KEY_TEMPERATURE:
        message.temperature = tuple->value->int32;
        break;
      case KEY_IS_DAYLIGHT:
        message.is_daylight = tuple->value->int32;
        break;
      case KEY_TICKER:
        message.ticker = tuple->value->cstring;
        break;
      case MESSAGE_KEY_TICKER_ON:
        message.show_ticker = tuple->value->int32;
        break;
      case MESSAGE_KEY_COIN:
        message.coin = atoi(tuple->value->cstring);
        set_clay_message(&message);
        break;
      case MESSAGE_KEY_CURRENCY:
        message.currency = atoi(tuple->value->cstring);
        set_clay_message(&message);
        break;
      case MESSAGE_KEY_SHOW_SECONDS_HAND:
        message.seconds_hand_mode = atoi(tuple->value->cstring);
        set_clay_message(&message);
        break;
     case MESSAGE_KEY_SECONDS_HAND_DURATION:
        message.seconds_hand_duration = tuple->value->int32;
        set_clay_message(&message);
        break;
      case MESSAGE_KEY_TEMPERATURE_UNITS:
        message.temperature_units = atoi(tuple->value->cstring);
        set_clay_message(&message);
        break;
      case MESSAGE_KEY_VIBRATE_ON_BLUETOOTH_DISCONNECT:
        message.vibrate_on_bluetooth_disconnect = tuple->value->int32;
        set_clay_message(&message);
      break;
      case MESSAGE_KEY_SHOW_TIMEZONE:
        message.show_timezone = tuple->value->int32;
        set_clay_message(&message);
      break;
      case MESSAGE_KEY_SHOW_BATTERY_AT_PERCENT:
        message.show_battery_at_percent = tuple->value->int32;
        set_clay_message(&message);
        break;
      case MESSAGE_KEY_HAND_STYLE:
        message.hand_style = atoi(tuple->value->cstring);
        set_clay_message(&message);
        break;
      case MESSAGE_KEY_TEMPERATURE_SIZE:
        message.temperature_font_size = atoi(tuple->value->cstring);
        set_clay_message(&message);
        break;
      case MESSAGE_KEY_BG_COLOR:
        message.bg_color = tuple->value->uint32;
        set_clay_message(&message);
        break;    
      case MESSAGE_KEY_FG1_COLOR:
        message.fg1_color = tuple->value->uint32;
        set_clay_message(&message);
        break;    
      case MESSAGE_KEY_FG2_COLOR:
        message.fg2_color = tuple->value->uint32;
        set_clay_message(&message);
        break;    
      case MESSAGE_KEY_FG3_COLOR:
        message.fg3_color = tuple->value->uint32;
        set_clay_message(&message);
        break;    
      case MESSAGE_KEY_WEATHER_QUIET_TIME:
        message.weather_quiet_time = tuple->value->int32;
        set_clay_message(&message);
        break;   
      case MESSAGE_KEY_WEATHER_QUIET_TIME_START:
        message.weather_quiet_time_start = tuple->value->int32;
        set_clay_message(&message);
        break;
      case MESSAGE_KEY_WEATHER_QUIET_TIME_STOP:
        message.weather_quiet_time_stop = tuple->value->int32;
        set_clay_message(&message);
        break;
      case MESSAGE_KEY_WEATHER_SOURCE:
        message.weather_source = atoi(tuple->value->cstring);
        set_clay_message(&message);
        break;
      default:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Application received unknown key: %lu", tuple->key);
        break;
    }
  }

  switch (message.message_type) {
    case MESSAGE_TYPE_READY:
      ready_received(watchface_window);
      break;
    case MESSAGE_TYPE_WEATHER:
      weather_received(watchface_window, &message);
      break;
    case MESSAGE_TYPE_SETTINGS:
       settings_received(watchface_window, &message);
       break;
    case MESSAGE_TYPE_TICKER:
      ticker_received(watchface_window, &message);
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Application received message of unknown type: %d ", message.message_type);
      break;
  }
}

Window *watchface_window_create() {
  Window *watchface_window = window_create();
  WatchfaceWindow *this = malloc(sizeof(*this));
  *this = (__typeof(*this)) {
    // IMPORTANT: Keep default values in sync with watchface_window.js.
    .seconds_hand_mode = persist_read_int_or_default(MESSAGE_KEY_SHOW_SECONDS_HAND, SECONDS_HAND_OFF),
    .seconds_hand_duration = persist_read_int_or_default(MESSAGE_KEY_SECONDS_HAND_DURATION, 2 ),
    .seconds_duration_timer = NULL,
    .most_recent_tap = 0,
    .ticker_currency = persist_read_int_or_default(MESSAGE_KEY_CURRENCY, 1),
    .ticker_coin = persist_read_int_or_default(MESSAGE_KEY_COIN, 1),
    .temperature_units = persist_read_int_or_default(MESSAGE_KEY_TEMPERATURE_UNITS, TEMPERATURE_UNITS_FAHRENHEIT),
    .vibrate_on_bluetooth_disconnect = persist_read_bool_or_default(MESSAGE_KEY_VIBRATE_ON_BLUETOOTH_DISCONNECT, true),
    .show_battery_at_percent = persist_read_int_or_default(MESSAGE_KEY_SHOW_BATTERY_AT_PERCENT, 40),
    .hand_style = persist_read_int_or_default(MESSAGE_KEY_HAND_STYLE, 1),
    .temperature_font_size = persist_read_int_or_default(MESSAGE_KEY_TEMPERATURE_SIZE, 2),
    .ticker_font_size = 2,
    .bg_color = persist_read_int_or_default(MESSAGE_KEY_BG_COLOR, 0x000055),    // OxfordBlue
    .fg1_color = persist_read_int_or_default(MESSAGE_KEY_FG1_COLOR, 0xAAAA55),  // Brass
    .fg2_color = persist_read_int_or_default(MESSAGE_KEY_FG2_COLOR, 0xFFFF55),  //  Iterine
    .fg3_color = persist_read_int_or_default(MESSAGE_KEY_FG3_COLOR, 0xFF0000),   // Red
   
    .weather_quiet_time = persist_read_int_or_default(MESSAGE_KEY_WEATHER_QUIET_TIME,0),
    .weather_quiet_time_start = persist_read_int_or_default(MESSAGE_KEY_WEATHER_QUIET_TIME_START, 23),
    .weather_quiet_time_stop = persist_read_int_or_default(MESSAGE_KEY_WEATHER_QUIET_TIME_STOP,6),
    .weather_source = persist_read_int_or_default(MESSAGE_KEY_WEATHER_SOURCE, WEATHER_SOURCE_OPENWEATHERMAP),

    .font_hours = NULL,
    .font_date = NULL,
    .font_ticker = NULL,
    .font_temperature = NULL,
    .font_temperature_small = NULL,
    .font_condition = NULL,
    .font_battery = NULL,
    .font_bluetooth = NULL,

    .background_layer = NULL,

    .date_text_layer = NULL,
    .date_text = "",

    .temperature_text_layer = NULL,
    .temperature_text = "",
      
    .ticker_text_layer = NULL,
    .ticker_text = "",

    .condition_text_layer = NULL,
    .condition_text = "",

    .battery_layer = NULL,
    .battery_text_layer = NULL,
    .battery_text = "",

    .bluetooth_text_layer = NULL,
    .bluetooth_text = "",
    
    .show_timezone = persist_read_bool_or_default(MESSAGE_KEY_SHOW_TIMEZONE, false),
    .timezone_text_layer = NULL,
    .timezone_text = "",

    .hands_layer = NULL,
    .second_hand_layer = NULL,

    .weather_update_timer = NULL,
    .weather_update_backoff_interval = -1,
    .expected_weather_message_id = 0,
    
    .show_ticker = persist_read_bool_or_default(MESSAGE_KEY_TICKER_ON, true),
    .ticker_update_timer = NULL,
    .ticker_update_backoff_interval = -1,
    .expected_ticker_message_id = 0,
  };
  window_set_user_data(watchface_window, this);

  window_set_window_handlers(watchface_window, (WindowHandlers) {
    .load = watchface_window_load,
    .appear = watchface_window_appear,
    .disappear = watchface_window_disappear,
    .unload = watchface_window_unload
  });

  app_message_set_context(watchface_window);
  app_message_register_inbox_received(inbox_received);
  app_message_register_outbox_sent(outbox_sent);
  app_message_register_outbox_failed(outbox_failed);
 //app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  app_message_open(1000,1000);

  g_watchface_window = watchface_window;
  return watchface_window;
}

void watchface_window_destroy(Window *watchface_window) {
  g_watchface_window = NULL;

  app_message_deregister_callbacks();

  WatchfaceWindow *this = window_get_user_data(watchface_window);
  free(this);

  window_destroy(watchface_window);
}

void watchface_window_show(Window *watchface_window) {
  window_stack_push(watchface_window, true);
}
