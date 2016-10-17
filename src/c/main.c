#include <pebble.h>
#include "button.h"

static Window *s_window;
static Layer *s_hands_layer, *s_act_layer;

bool isLastMinute = true;
bool forceRefresh = true;
// int g_max_data_point = -1;
int g_max_data_point = 60;
int g_data_array[MAX_ARRAY_NUM] = {0}; 
// int g_data_array[MAX_ARRAY_NUM] = {10,10,40,80,70,50,90,30,20,0,0,0,10,20,40,50,60,70,20,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,100,20,40,20,60,30,50,50,60,90,30,10,15,5,5,5,0,0,0,0,40,40,30};
GRect g_watch_bound;
GPoint g_watch_center;
float h, m, s;
GTextAttributes * g_text_attr;

static void draw_line(GContext *ctx, GRect bounds, GColor color, GPoint *center, float radiusFraction, float angle)
{
  int r = bounds.size.w;
  float diff = (r - r * radiusFraction) / 2;

  graphics_context_set_stroke_color(ctx, color);

  float trigangle = DEG_TO_TRIGANGLE(angle);
  GPoint p1 = gpoint_from_polar(GRect(bounds.origin.x + diff, bounds.origin.y + diff, r * radiusFraction, r * radiusFraction), GOvalScaleModeFitCircle, trigangle);
  graphics_draw_line(ctx, *center, p1);
}

// For more robust application, should check if there are such many available records
static uint32_t get_available_records(HealthMinuteData *array, time_t query_start, 
                                      time_t query_end, uint32_t max_records) {
  time_t next_start = query_start;
  time_t next_end = query_end;
  uint32_t num_records_found = 0;

  // Find more records until no more are returned
  while (num_records_found < max_records) {
    int ask_num_records = max_records - num_records_found;
    uint32_t ret_val = health_service_get_minute_history(&array[num_records_found], 
                                        ask_num_records, &next_start, &next_end);
    if (ret_val == 0) {
      // a 0 return value means no more data is available
      return num_records_found;
    }
    num_records_found += ret_val;
    next_start = next_end;
    next_end = query_end;
  } 

  return num_records_found;
}

static int get_sum(HealthMinuteData * data, int num) {
  int sum = 0;
  for (int i = 0; i < num; i ++)
    sum += data[i].is_invalid ? 0 : data[i].steps;
  return sum;
} 


static void set_data_from_record(time_t query_start, time_t query_end, int * step_count) {
  uint32_t max_records = (query_end - query_start) / SECONDS_PER_MINUTE;
  HealthMinuteData *data = 
    (HealthMinuteData*)malloc(max_records * sizeof(HealthMinuteData));
  
  max_records = get_available_records(data, query_start, query_end, max_records);
  
  
  int sum_over_num_of_records = isLastMinute ? 1 : 12;
  
  uint32_t count = 0;
  HealthMinuteData * p_data = data;

  APP_LOG(APP_LOG_LEVEL_INFO, "g_max_data_point before: %d", g_max_data_point);
  g_max_data_point = -1;
  for (int i = 0; i < MAX_ARRAY_NUM; i ++)
    step_count[i] = 0;

  while (count < max_records) {
    int rest_count = (int)(max_records - count);
    int sum_count = (rest_count < sum_over_num_of_records) ? rest_count : sum_over_num_of_records;
    int sum = get_sum(p_data, sum_count);
    
    (*step_count) = sum;

    step_count ++;
    p_data += sum_count;
    count += sum_count;
    
    //APP_LOG(APP_LOG_LEVEL_INFO, "count: %d", (int)count);
    g_max_data_point = (sum > g_max_data_point) ? sum : g_max_data_point;
  }
  APP_LOG(APP_LOG_LEVEL_INFO, "g_max_data_point after: %d", g_max_data_point);
  
  free(data);
}

// Draw the step counts on the watch face
static void draw_activity(GContext *ctx, int cur_time) {
  if (g_max_data_point <= 0)
    return;
  
  graphics_context_set_text_color(ctx, GColorDarkGray);
  graphics_draw_text(ctx, isLastMinute ? "Past 1 Hour" : "Past 12 hours", fonts_get_system_font(FONT_KEY_GOTHIC_18),
                    GRect(g_watch_center.x - 40, g_watch_center.y + 40, 80, 20), GTextOverflowModeWordWrap,
                    GTextAlignmentCenter, g_text_attr);
  
  int r_outer_circle = g_watch_bound.size.w / 2 - MIN_ACTIVITY_LENGTH;
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  
  for (int i = 0; i < MAX_ARRAY_NUM; i ++) {
    float deg = 360.f * ((cur_time + i) % 60) / 60.f;
    
    float trigangle = DEG_TO_TRIGANGLE(deg);
    float perc = (float)g_data_array[i] / g_max_data_point;
    perc = (perc < 0.1f) ? 0.1f : perc;
    int r_inner_circle = (int)(g_watch_bound.size.w / 2 - perc * MAX_ACTIVITY_LENGTH);
//     GPoint p1 = gpoint_from_polar(g_watch_bound, GOvalScaleModeFitCircle, trigangle);
    
//     if (r_inner_circle < 0)
//      APP_LOG(APP_LOG_LEVEL_INFO, "g_data_array[i]: %d g_max_data_point: %d", g_data_array[i], g_max_data_point);
    
    GPoint p1 = gpoint_from_polar(GRect(g_watch_center.x - r_outer_circle, g_watch_center.y - r_outer_circle,
                                        r_outer_circle * 2, r_outer_circle * 2),
                                  GOvalScaleModeFitCircle, trigangle);
    
    GPoint p2 = gpoint_from_polar(GRect(g_watch_center.x - r_inner_circle, g_watch_center.y - r_inner_circle,
                                        r_inner_circle * 2, r_inner_circle * 2),
                                  GOvalScaleModeFitCircle, trigangle);
    graphics_draw_line(ctx, p1, p2);
  }
}


static void hands_update_proc(Layer *layer, GContext *ctx) {

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  /* Update the hand */
  s = 360.f * t->tm_sec;
  s /= 60.f;

  m = 360.f * t->tm_min;
  m /= 60.f;

  h = 360.f * (t->tm_hour % 12);
  h /= 12.f;
  h += m / 12;

  graphics_context_set_stroke_width(ctx, 5);
  draw_line(ctx, g_watch_bound, GColorBlack, &g_watch_center, 0.8f, m);
  draw_line(ctx, g_watch_bound, GColorBlack, &g_watch_center, 0.4f, h);

  graphics_context_set_stroke_width(ctx, 1);
  draw_line(ctx, g_watch_bound, GColorBlack, &g_watch_center, 1.0f, s);  
  
  /* Update the step count activity */
  if (isLastMinute && (t->tm_sec == 0 || forceRefresh)) {
    // In past an hour
    time_t query_end = time(NULL); // - (15 * SECONDS_PER_MINUTE);
    time_t query_start = query_end - SECONDS_PER_HOUR;
    APP_LOG(APP_LOG_LEVEL_INFO, "act_update_proc per minute");
  
    set_data_from_record(query_start, query_end, g_data_array);
    if (forceRefresh)
      forceRefresh = false;
  } else if (!isLastMinute && (((t->tm_min % 12) == 0 && t->tm_sec == 0) || forceRefresh)) {
    // In past 12 hours
    time_t query_end = time(NULL);
    time_t query_start = query_end - SECONDS_PER_HOUR * 12;
    APP_LOG(APP_LOG_LEVEL_INFO, "act_update_proc per 12 minutes");
    
    set_data_from_record(query_start, query_end, g_data_array);
    if (forceRefresh)
      forceRefresh = false;
  }
  
  int cur_time = isLastMinute ? t->tm_min : (t->tm_hour + t->tm_min / 12);
  draw_activity(ctx, cur_time);
}

static void handle_refresh_hands(struct tm *tick_time, TimeUnits units_changed) {
 layer_mark_dirty(window_get_root_layer(s_window));
  layer_mark_dirty(s_hands_layer);
}

static void window_load(Window *window) {
  forceRefresh = true;
  
  g_text_attr = graphics_text_attributes_create();
  
  Layer *window_layer = window_get_root_layer(window);
  g_watch_bound = layer_get_bounds(window_layer);
  g_watch_center = grect_center_point(&g_watch_bound);
  
  s_hands_layer = layer_create(g_watch_bound);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);
  
  s_act_layer = layer_create(g_watch_bound);
  layer_add_child(window_layer, s_act_layer);
}

static void window_unload(Window *window) {
  graphics_text_attributes_destroy(g_text_attr);
  
  layer_destroy(s_hands_layer);
  layer_destroy(s_act_layer);
}


static void init() {
  s_window = window_create();
  window_set_click_config_provider(s_window, click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
  tick_timer_service_subscribe(SECOND_UNIT, handle_refresh_hands);
}

static void deinit() {
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
