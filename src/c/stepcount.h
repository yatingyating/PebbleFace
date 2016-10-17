#pragma once

#include <pebble.h>
#define TARGET_STEPS 8000

extern Window * step_window;
extern Layer * step_shadow_layer;;

void step_handle_init();
void step_handle_deinit();
void step_window_handler(struct tm *tick_time, TimeUnits units_changed);
