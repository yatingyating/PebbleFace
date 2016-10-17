#include <pebble.h>
#include "hands.h"
#include "stepcount.h"

Window *window;
bool bShowCount = false;  // if true, show the real number of steps

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Toggle the button to show the current number of steps
  bShowCount = !bShowCount;
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

GPoint get_point_on_polar_from_r(GRect bounds, int radius, float angle) {
  int w = bounds.size.w;
  int diameter = radius * 2;
  float diff = (w - diameter) / 2;
  
  float trigangle = DEG_TO_TRIGANGLE(angle);
  GPoint p1 = gpoint_from_polar(GRect(bounds.origin.x + diff, bounds.origin.y + diff, diameter, diameter), GOvalScaleModeFitCircle, trigangle);
  return p1;
}

void window_load(Window *window) {
  hands_init();
}

void window_unload(Window *window) {
  hands_deinit();
}

void handle_init(void) {
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  window_set_background_color(window, GColorCeleste);
  window_stack_push(window, true);
  
  step_window_init();
}

void handle_deinit(void) {
  step_window_deinit();
  window_destroy(window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
