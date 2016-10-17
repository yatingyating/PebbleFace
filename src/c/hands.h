#pragma once

#include <pebble.h>

extern Layer *graphics_layer;

extern int hour;
extern int minutes;
extern int seconds;
extern float hour_angle;
extern const int thres_delta_steps;

void hands_init();
void hands_deinit();
int get_step_count_today();
void enable_step_window();
void tick_handler(struct tm *tick_time, TimeUnits units_changed);