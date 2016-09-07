#include "watchface_window.h"

int main() {
  Window *watchface_window = watchface_window_create();
  watchface_window_show(watchface_window);
  app_event_loop();
  watchface_window_destroy(watchface_window);
}
