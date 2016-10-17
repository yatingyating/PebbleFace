#include "main_window.h"

static Window *s_window;
static Layer *s_canvas;

static int s_hours, s_minutes;

static int32_t get_angle_for_hour(int hour) {
  // Progress through 12 hours, out of 360 degrees
  return (hour * 360) / 12;
}

static int32_t get_angle_for_minute(int minute) {
  // Progress through 60 minutes, out of 360 degrees
  return (minute*360) / 60;
}

static void layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GRect frame;
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  
  // step history
  graphics_context_set_fill_color(ctx, GColorIcterine);
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int sec_in_this_hour = t->tm_sec + (t->tm_min) * 60;
  int hour_12h = t->tm_hour % 12;
  int max_records = 60;
  time_t end;
  time_t start;
  int steps;
  HealthMinuteData minute_data[max_records];
  for (int i=0; i<12; i++){
    float hour_angle = i * 30 - 0.5;
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
    float step_ratio = (float)((steps>STEP_GOAL)?STEP_GOAL:steps)/STEP_GOAL;
    int step_radius = (int)(step_ratio*(bounds.size.h/2-6))+6;
    int step_inset = bounds.size.h/2-step_radius;
    frame = grect_inset(bounds, GEdgeInsets(step_inset));
    graphics_fill_radial(ctx, frame, GOvalScaleModeFitCircle, step_radius, DEG_TO_TRIGANGLE(hour_angle), DEG_TO_TRIGANGLE(hour_angle+31));
  } 
  
  // 12 hours only, with a minimum size
  s_hours -= (s_hours > 12) ? 12 : 0;

  // Minutes are expanding circle arc
  int minute_angle = get_angle_for_minute(s_minutes);
  frame = grect_inset(bounds, GEdgeInsets(5*INSET));

  GPoint pos = gpoint_from_polar(frame, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(minute_angle));
  graphics_context_set_fill_color(ctx, GColorChromeYellow);
  graphics_fill_circle(ctx, pos, 8); 

  // Hours are dots
  for(int i = 0; i < 12; i++) {
    int hour_angle = get_angle_for_hour(i);
    pos = gpoint_from_polar(frame, GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(hour_angle));

    graphics_context_set_fill_color(ctx, i <= s_hours ? HOURS_COLOR : HOURS_COLOR_INACTIVE);
    graphics_fill_circle(ctx, pos, HOURS_RADIUS); 
  }
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_canvas = layer_create(bounds);
  layer_set_update_proc(s_canvas, layer_update_proc);
  layer_add_child(window_layer, s_canvas);
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas);
  window_destroy(s_window);
}

void main_window_push() {
  s_window = window_create();
  window_set_background_color(s_window, BG_COLOR);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}

void main_window_update(int hours, int minutes) {
  s_hours = hours;
  s_minutes = minutes;
  layer_mark_dirty(s_canvas);
}
