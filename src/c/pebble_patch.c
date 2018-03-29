#include "pebble_patch.h"

// Note that we are actually storing the settings values twice:  Once on the phone as part of Clay, and once on the watch as part of persistent storage.
//  This is on purpose to allow the settings to be retained when the app starts even if the phone is currently disconnected, but does have a potential for
// them getting out of sync.   If they do get out of sync, they should be fixed the next time that clay is run for settings.

bool persist_read_bool_or_default(uint32_t key, bool default_value) {
  return persist_exists(key) ? persist_read_bool(key) : default_value;
}

int32_t persist_read_int_or_default(uint32_t key, int32_t default_value) {
  return persist_exists(key) ? persist_read_int(key) : default_value;
}

char *persist_read_string_or_default(uint32_t key, char* default_value) {
  static char buffer[200];
  if ( persist_exists(key) ) {
    //snprintf(s_buffer, sizeof(s_buffer), "Location: %s", location_name);
    persist_read_string(key, buffer, sizeof(buffer));
    return (char*)buffer;
  } else {
    return default_value;
  }
}

//pointer issues
char *persist_read_string_or_default1(uint32_t key, char* default_value) {
  static char buffer1[200];
  if ( persist_exists(key) ) {
    //snprintf(s_buffer, sizeof(s_buffer), "Location: %s", location_name);
    persist_read_string(key, buffer1, sizeof(buffer1));
    return (char*)buffer1;
  } else {
    return default_value;
  }
}

struct tm* local_time_peek() {
  time_t utc_time = time(NULL);
  return localtime(&utc_time);
}
