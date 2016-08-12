#pragma once

#include <pebble.h>

Window *watchface_window_create();
void watchface_window_destroy(Window *watchface_window);
void watchface_window_show(Window *watchface_window);
