#include <pebble.h>

// #define MAX_ARRAY_NUM 40
#define MAX_ARRAY_NUM 60
#define MAX_ACTIVITY_LENGTH 30
#define MIN_ACTIVITY_LENGTH 5

extern bool isLastMinute;
extern bool forceRefresh;
extern int g_max_data_point;
extern int g_data_array[MAX_ARRAY_NUM];
extern GRect g_watch_bound;
extern GPoint g_watch_center;
extern float h, m, s;
  
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "clicked selected button");
  isLastMinute = !isLastMinute;
  forceRefresh = true;
  APP_LOG(APP_LOG_LEVEL_INFO, "g_watch_center: %d %d", g_watch_center.x, g_watch_center.y);
}


static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

