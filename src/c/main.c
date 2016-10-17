#include <pebble.h>
#include "main.h"
#include "hands.h"
#include "stepcount.h"

Window *window;
bool bShowCount = false;

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Toggle to go to the step counts window
  enable_step_window();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
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

  window_set_background_color(window, GColorWhite);
  window_stack_push(window, true);
}

void handle_deinit(void) {
  window_destroy(window);
}

int main(void) {
  handle_init();
  step_handle_init();
  app_event_loop();
  step_handle_deinit();
  handle_deinit();
}
