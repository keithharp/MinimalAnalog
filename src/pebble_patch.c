#include "pebble_patch.h"

bool persist_read_bool_or_default(uint32_t key, bool default_value) {
  return persist_exists(key) ? persist_read_bool(key) : default_value;
}

int32_t persist_read_int_or_default(uint32_t key, int32_t default_value) {
  return persist_exists(key) ? persist_read_int(key) : default_value;
}

struct tm* local_time_peek() {
  time_t utc_time = time(NULL);
  return localtime(&utc_time);
}
