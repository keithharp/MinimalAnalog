#pragma once

#include <pebble.h>

#define SCREEN_FRAME GRect(0, 0, 144, 168)

#define rot_bitmap_layer_get_layer(rot_bitmap_layer) ((Layer *)(rot_bitmap_layer))
#define rot_bitmap_layer_set_compositing_mode rot_bitmap_set_compositing_mode
#define rot_bitmap_layer_set_src_ic rot_bitmap_set_src_ic

bool persist_read_bool_or_default(uint32_t key, bool default_value);
int32_t persist_read_int_or_default(uint32_t key, int32_t default_value);
char *persist_read_string_or_default1(uint32_t key, char* default_value);
char *persist_read_string_or_default(uint32_t key, char* default_value);

struct tm* local_time_peek();
