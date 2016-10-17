#include <pebble.h>
#include "hands.h"

Layer *graphics_layer;
int hour, minutes, seconds;

GRect getRectFraction(GRect bounds, float fraction) {
  int r = bounds.size.w;
  float diff = (r - r * fraction) / 2;

  return GRect(bounds.origin.x + diff, bounds.origin.y + diff, r * fraction, r * fraction);
}


void draw_line(GContext *ctx, GRect bounds, GColor color, GPoint *center, int length, float angle)
{
  GPoint p1 = get_point_on_polar_from_r(bounds, length, angle);
  
  graphics_context_set_stroke_color(ctx, color);
  graphics_draw_line(ctx, *center, p1);
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
  
  if (s == 0) {
    refresh_step_window(h);
  }

  graphics_context_set_stroke_width(ctx, 5);
  draw_line(ctx, bounds, GColorBlack, &p0, 55, m);
  draw_line(ctx, bounds, GColorDarkGray, &p0, 36, h);

  graphics_context_set_stroke_width(ctx, 1);
  draw_line(ctx, bounds, GColorDarkGray, &p0, 70, s);
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
