#include <pebble.h>
#include "main.h"
#include "hands.h"
#include "stepcount.h"

Layer *graphics_layer;
int hour, minutes, seconds;
int old_step = 0;
const int thres_delta_steps = 100;

int get_step_count_today() {
  return health_service_sum_today(HealthMetricStepCount);
}

void enable_step_window() {
  bShowCount = true;
  tick_timer_service_subscribe(MINUTE_UNIT, step_window_handler); // call every time switch between different windows  
  window_stack_push(step_window, false);
}

GPoint get_point_on_polar_from_r(GRect bounds, int radius, float angle) {
  int w = bounds.size.w;
  int diameter = radius * 2;
  float diff = (w - diameter) / 2;
  
  float trigangle = DEG_TO_TRIGANGLE(angle);
  GPoint p1 = gpoint_from_polar(GRect(bounds.origin.x + diff, bounds.origin.y + diff, diameter, diameter), GOvalScaleModeFitCircle, trigangle);
  return p1;
}

void draw_line(GContext *ctx, GRect bounds, GColor color, GPoint *center, int length, float angle)
{
  GPoint p1 = get_point_on_polar_from_r(bounds, length, angle);
  
  graphics_context_set_stroke_color(ctx, color);
  graphics_draw_line(ctx, *center, p1);
}

void timer_callback() {
  // If the increased step counts is larger than a threshold, then show the step count screen
  int new_step = get_step_count_today();
  if (new_step - old_step > thres_delta_steps && !bShowCount) {
    old_step = new_step;
    enable_step_window();
  } else {
    old_step = new_step;
  }
}


void update_graphics(Layer *layer, GContext *ctx) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint p0 = grect_center_point(&bounds);

  float s = 360.f * seconds;
  s /= 60.f;

  float m = 360.f * minutes;
  m /= 60.f;

  float h = 360.f * (hour % 12);
  h /= 12.f;

  h += m / 12;
  
  graphics_context_set_stroke_width(ctx, 5);
  draw_line(ctx, bounds, GColorBlack, &p0, 45, m);
  draw_line(ctx, bounds, GColorDarkGray, &p0, 30, h);

  graphics_context_set_stroke_width(ctx, 1);
  draw_line(ctx, bounds, GColorDarkGray, &p0, 60, s);
  
  if (s == 0)
    app_timer_register(2000, timer_callback, NULL);
}

void update_time() {
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  hour = tick_time->tm_hour;
  minutes = tick_time->tm_min;
  seconds = tick_time->tm_sec;
  
  layer_mark_dirty(graphics_layer);
}

void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

void hands_init() {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  graphics_layer = layer_create(bounds);
  layer_set_update_proc(graphics_layer, update_graphics);
  layer_add_child(window_layer, graphics_layer);

  update_time();
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

void hands_deinit() {
  tick_timer_service_unsubscribe();
  layer_destroy(graphics_layer);
}
