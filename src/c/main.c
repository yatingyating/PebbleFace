#include <pebble.h>
#include <stdlib.h>
#include <time.h>
#include "analog.h"

static Window *s_window;
static Layer *s_simple_bg_layer, *s_hands_layer;

static GPath *s_minute_arrow, *s_hour_arrow;


static void bg_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GRect frame = grect_inset(bounds, GEdgeInsets(8));
  graphics_context_set_fill_color(ctx, GColorBabyBlueEyes);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  
  // hour dots
  for (int i=0; i<12; i++){
    int hour_angle = i * 360 / 12;
    GRect frame = grect_inset(bounds, GEdgeInsets(25));
    GPoint pos = gpoint_from_polar(frame, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(hour_angle));
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_circle(ctx, pos, 3);
  }

  // step history
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int sec_in_this_hour = t->tm_sec + (t->tm_min) * 60;
  int hour_12h = t->tm_hour % 12;
  int max_records = 60;
  time_t end;
  time_t start;
  int steps;
  HealthMinuteData minute_data[max_records];
  graphics_context_set_fill_color(ctx, GColorDarkGray);

  for (int i=0; i<12; i++){
    int hour_angle = i * 30;
    if(i < hour_12h){
      start = now-sec_in_this_hour-(hour_12h-i)*3600;
      end = now-sec_in_this_hour-(hour_12h-i-1)*3600;
    }
    else if(i == hour_12h){
      start = now-sec_in_this_hour;
      end = now;
    }
    else if(i > hour_12h){
      start = now-sec_in_this_hour-(hour_12h-i+12)*3600;
      end = now-sec_in_this_hour-(hour_12h-i+12-1)*3600;
    }
    start -= 15*60;
    end -= 15*60;
    if ((persist_exists(i)) && (i!=hour_12h)){
      steps = persist_read_int(i);
    }
    else {
      steps = 0;
      int num_records = health_service_get_minute_history(&minute_data[0], max_records, &start, &end);
      for(int j=0;j<num_records;j++){
        steps += (int)minute_data[j].steps;
      persist_write_int(i, steps);
      }
    }
    float step_angle = (float)((steps>STEP_GOAL)?STEP_GOAL:steps)/STEP_GOAL*30;
    step_angle = (step_angle>30)?30:step_angle;
    graphics_fill_radial(ctx, frame, GOvalScaleModeFitCircle, 5, DEG_TO_TRIGANGLE(hour_angle), DEG_TO_TRIGANGLE(hour_angle+step_angle));
  }
}


static void min_update_proc(Layer *layer, GContext *ctx){
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  // minute/hour hand
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  
  gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_arrow);
  gpath_draw_outline(ctx, s_hour_arrow);
  
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  
  gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_arrow);
  gpath_draw_outline(ctx, s_minute_arrow);
    
  // ring in the middle
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, 3);
  
}


static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  Layer *window_layer = window_get_root_layer(s_window);
  layer_mark_dirty(window_layer);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, s_simple_bg_layer);

  s_hands_layer= layer_create(bounds);
  layer_set_update_proc(s_hands_layer, min_update_proc);
  layer_add_child(window_layer, s_hands_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_simple_bg_layer);
  layer_destroy(s_hands_layer);
}

static void init() {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  // init hand paths
  s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);

  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);
  gpath_move_to(s_minute_arrow, center);
  gpath_move_to(s_hour_arrow, center);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}

static void deinit() {
  gpath_destroy(s_minute_arrow);
  gpath_destroy(s_hour_arrow);

  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}