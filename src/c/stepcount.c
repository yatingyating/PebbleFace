#include "stepcount.h"

//Window * step_window;
Layer * step_graphics_layer;
TextLayer *step_text_layer;
int TARGET_STEPS = 8000;

float hour_angle = 0;
int step_sum;


// For more robust application, should check if there are such many available records
uint32_t get_available_records(HealthMinuteData *array, time_t query_start, 
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

/*int get_sum(HealthMinuteData * data, int num) {
  int sum = 0;
  for (int i = 0; i < num; i ++)
    sum += data[i].is_invalid ? 0 : data[i].steps;
  return sum;
}

int get_step_count(time_t query_start, time_t query_end) {
  uint32_t max_records = (query_end - query_start) / SECONDS_PER_MINUTE;
  HealthMinuteData *data = 
    (HealthMinuteData*)malloc(max_records * sizeof(HealthMinuteData));
  
  max_records = get_available_records(data, query_start, query_end, max_records);
  
  int sum = get_sum(data, max_records);
  free(data);
  
  return sum;
}*/

void get_change_point(time_t query_start, time_t query_end, int * change_points, int numLevel) {
  uint32_t max_records = (query_end - query_start) / SECONDS_PER_MINUTE;
  HealthMinuteData *data = 
    (HealthMinuteData*)malloc(max_records * sizeof(HealthMinuteData));
  
  max_records = get_available_records(data, query_start, query_end, max_records);
  
  int sum = 0;
  int step = TARGET_STEPS / numLevel;
  int nextLevel = step;
  int count = 0;
  for (int i = 0; i < (int)max_records; i ++) {
    sum += data[i].is_invalid ? 0 : data[i].steps;
    if (sum > nextLevel) {
      change_points[count] = i;
      nextLevel += step;
      count ++;
      if (count == numLevel)
        break;
    }
  }
}


int time_to_color_point(struct tm *tick_time) {
  return tick_time->tm_hour * 60 + tick_time->tm_min;
}



GColor8 color_array[NUM_COLOR_POITNS];

void draw_arc(int angle_start_idx, int angle_end_idx, GPoint center, GContext * ctx, GColor8 color) {
  int r;
  float angle_start = (angle_start_idx % 720) / (float)2;
  float angle_end = (angle_end_idx % 720) / (float)2;
  graphics_context_set_stroke_color(ctx, color);
  if (angle_end_idx > 720 && angle_start_idx < 720) {
    r = RADIUS_1;
    graphics_draw_arc(ctx, GRect(center.x - r, center.y - r, 2 * r, 2 * r), GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(angle_start), DEG_TO_TRIGANGLE(360));
    r = RADIUS_2;
    graphics_draw_arc(ctx, GRect(center.x - r, center.y - r, 2 * r, 2 * r), GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(angle_end));
  } else if (angle_end_idx < 720) {
    r = RADIUS_1;
    graphics_draw_arc(ctx, GRect(center.x - r, center.y - r, 2 * r, 2 * r), GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(angle_start), DEG_TO_TRIGANGLE(angle_end));
  } else {
    r = RADIUS_2;
    graphics_draw_arc(ctx, GRect(center.x - r, center.y - r, 2 * r, 2 * r), GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(angle_start), DEG_TO_TRIGANGLE(angle_end));
  }
}

int get_step_count_today() {
  return health_service_sum_today(HealthMetricStepCount);
}

void step_window_update(Layer * layer, GContext * ctx) {
  step_sum = get_step_count_today();
  
  // update the color of points on the circle in real time
  GColor8 color;
  if (step_sum < TARGET_STEPS / 3)
    color = GColorLightGray;
  else if (step_sum < TARGET_STEPS / 3 * 2)
    color = GColorDarkGray;
  else if (step_sum < TARGET_STEPS)
    color = GColorBlack;
  else
    color = GColorRed;
  
  time_t query_end = time(NULL);  // current time
  struct tm *cur_time = localtime(&query_end);
  int cur_point = time_to_color_point(cur_time);
  color_array[cur_point] = color;
    
  GRect bounds = layer_get_bounds(step_graphics_layer); 
  GPoint center = grect_center_point(&bounds);
  
  
  int angle_start_idx = 0;
  GColor8 arc_color = GColorLightGray;
  for (int i = 0; i <= cur_point; i ++) {
    if (gcolor_equal(color_array[i], arc_color)) {
      continue;
    } else {
      draw_arc(angle_start_idx, i - 1, center, ctx, arc_color);
      angle_start_idx = i;
      arc_color = color_array[i];
    }
  }
  draw_arc(angle_start_idx, cur_point, center, ctx, arc_color);
  draw_arc(cur_point, 1440-1, center, ctx, GColorWhite);
  
  // add reference anchor (current anchors are hard-coded in the main.h)
  graphics_context_set_fill_color(ctx, GColorLightGray);
  GPoint p_1_3 = get_point_on_polar_from_r(bounds, RADIUS_2, (RECORD_1_3 % 720) / (float)2);
  graphics_fill_circle(ctx, p_1_3, 2);
  graphics_context_set_fill_color(ctx, GColorDarkGray);  
  GPoint p_2_3 = get_point_on_polar_from_r(bounds, RADIUS_2, (RECORD_2_3 % 720) / (float)2);
  graphics_fill_circle(ctx, p_2_3, 2);
  graphics_context_set_fill_color(ctx, GColorBlack);  
  GPoint p_3_3 = get_point_on_polar_from_r(bounds, RADIUS_2, (RECORD_3_3 % 720) / (float)2);
  graphics_fill_circle(ctx, p_3_3, 2);
  
  if (bShowCount) {
    static char labelText[10];
    snprintf(labelText, sizeof(labelText), "%d", step_sum);
    // APP_LOG(APP_LOG_LEVEL_DEBUG, "text: %c %c %c %c", labelText[0], labelText[1], labelText[2], labelText[3]);
    text_layer_set_text(step_text_layer, labelText);
  } else {
    text_layer_set_text(step_text_layer, "");
  }
}

void refresh_step_window(float hour) {
  hour_angle = hour;
  layer_mark_dirty(step_graphics_layer);
}


void step_window_init() {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  step_graphics_layer = layer_create(bounds);
  layer_add_child(window_layer, step_graphics_layer);
  layer_set_update_proc(step_graphics_layer, step_window_update);
  
  
  // Initialize data given the step counts today
  int change_points[3] = {NUM_COLOR_POITNS, NUM_COLOR_POITNS, NUM_COLOR_POITNS};
  get_change_point(time_start_of_today(), time(NULL), change_points, 3);
  for (int i = 0; i < change_points[0]; i ++)
    color_array[i] = GColorLightGray;
  for (int i = change_points[0]; i < change_points[1]; i ++)
    color_array[i] = GColorDarkGray;
  for (int i = change_points[1]; i < change_points[2]; i ++)
    color_array[i] = GColorBlack;
  for (int i = change_points[2]; i < NUM_COLOR_POITNS; i ++)
    color_array[i] = GColorRed;

  // Toggle the button to show the current number of steps
  GPoint center = grect_center_point(&bounds);
  step_text_layer = text_layer_create(GRect(center.x - 40, center.y + 40, 80, 20));
  text_layer_set_text_alignment(step_text_layer, GTextAlignmentCenter);
  text_layer_set_background_color(step_text_layer, GColorClear);
  text_layer_set_text_color(step_text_layer, GColorBlack);
  text_layer_set_font(step_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(step_text_layer));
}

void step_window_deinit() {
  layer_destroy(step_graphics_layer);
  text_layer_destroy(step_text_layer);
}
