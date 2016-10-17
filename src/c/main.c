#include <pebble.h>
#include "main.h"

#define SUN_RISE 6
#define SUN_SET 18

static GBitmap *s_bitmap_sun, *s_bitmap_moon;
static Window *s_window;
static BitmapLayer *s_bitmap_layer;
static Layer *s_simple_bg_layer, *s_hands_layer;
static TextLayer *s_text_layer, *s_text_layer2;
static GPath *s_tick_paths[NUM_CLOCK_TICKS];

float h, h2, m, s;
bool isNight;
GRect g_watch_bound;
GPoint g_watch_center;
GPoint g_time_zone_tick;

int BITMAP_SIZE = 20;
float time_zone_tick_perc = 0.4f;
bool isDemo = false;

// Draw the analog clock ticks
static void bg_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorWhite);
  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    const int x_offset = PBL_IF_ROUND_ELSE(18, 0);
    const int y_offset = PBL_IF_ROUND_ELSE(6, 0);
    gpath_move_to(s_tick_paths[i], GPoint(x_offset, y_offset));
    gpath_draw_filled(ctx, s_tick_paths[i]);
  }
}

static void draw_line(GContext *ctx, GRect bounds, GColor color, GPoint *center, float radiusFraction, float angle, bool isSetTick)
{
  int r = bounds.size.w;
  float diff = (r - r * radiusFraction) / 2;

  graphics_context_set_stroke_color(ctx, color);

  float trigangle = DEG_TO_TRIGANGLE(angle);
  GPoint p1 = gpoint_from_polar(GRect(bounds.origin.x + diff, bounds.origin.y + diff, r * radiusFraction, r * radiusFraction), GOvalScaleModeFitCircle, trigangle);
  graphics_draw_line(ctx, *center, p1);
  if (isSetTick)
    g_time_zone_tick = p1;
}

static void hands_update_proc(Layer *layer, GContext *ctx) {

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  s = 360.f * t->tm_sec;
  s /= 60.f;

  m = 360.f * t->tm_min;
  m /= 60.f;

  h = 360.f * (t->tm_hour % 12);
  h /= 12.f;
  h += m / 12;
  
  // Time in Beijing
  int hr2 = (t->tm_hour + 15) % 24;
  if (isDemo) 
    isNight = (s < 90) || (s >= 270);
  else
    isNight = (hr2 < SUN_RISE) || (hr2 >= SUN_SET);
  
  h2 = 360.f * (hr2 % 12);
  h2 /= 12.0f;
  h2 += m / 12;
  
  // Update hands
  if (!isDemo) {
    graphics_context_set_stroke_width(ctx, 5);
    draw_line(ctx, g_watch_bound, GColorWhite, &g_watch_center, 0.8f, m, false);
    draw_line(ctx, g_watch_bound, GColorWhite, &g_watch_center, 0.4f, h, false);
    draw_line(ctx, g_watch_bound, GColorLightGray, &g_watch_center, time_zone_tick_perc, h2, true);
  
    graphics_context_set_stroke_width(ctx, 1);
    draw_line(ctx, g_watch_bound, GColorWhite, &g_watch_center, 1.0f, s, false);    
  } else {
    // For demo purpose, set the bitmap rotating with the seconds hand
    graphics_context_set_stroke_width(ctx, 1);
    draw_line(ctx, g_watch_bound, GColorWhite, &g_watch_center, 0.4f, s, true);
  }
  
  // Update the bitmap layer (sun/moon)
  float time_zone_tick_length = time_zone_tick_perc * g_watch_bound.size.w;
  int x = (int)((g_time_zone_tick.x - g_watch_center.x) / time_zone_tick_length * BITMAP_SIZE * 1.4) + g_time_zone_tick.x;
  int y = (int)((g_time_zone_tick.y - g_watch_center.y) / time_zone_tick_length * BITMAP_SIZE * 1.4) + g_time_zone_tick.y;
  layer_set_frame((Layer *)s_bitmap_layer, GRect(x - BITMAP_SIZE/2, y - BITMAP_SIZE/2, BITMAP_SIZE, BITMAP_SIZE));
  bitmap_layer_set_bitmap(s_bitmap_layer, isNight ? s_bitmap_moon : s_bitmap_sun);
  layer_mark_dirty((Layer *)s_bitmap_layer);
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(s_window));
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  g_watch_bound = layer_get_bounds(window_layer);
  g_watch_center = grect_center_point(&g_watch_bound);

  s_simple_bg_layer = layer_create(g_watch_bound);
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, s_simple_bg_layer);

  s_hands_layer = layer_create(g_watch_bound);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);

  
  s_text_layer = text_layer_create(GRect(g_watch_center.x - 40, g_watch_center.y + 40, 80, 20));
  text_layer_set_text(s_text_layer, "BEIJING");
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_text_layer, GColorClear);
  text_layer_set_text_color(s_text_layer, GColorLightGray);
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer));
  
  
  s_text_layer2 = text_layer_create(GRect(g_watch_center.x - 40, g_watch_center.y - 60, 80, 20));
  text_layer_set_text(s_text_layer2, "US-CA");
  text_layer_set_text_alignment(s_text_layer2, GTextAlignmentCenter);
  text_layer_set_background_color(s_text_layer2, GColorClear);
  text_layer_set_text_color(s_text_layer2, GColorWhite);
  text_layer_set_font(s_text_layer2, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer2));
  
  s_bitmap_sun = gbitmap_create_with_resource(RESOURCE_ID_SUN);
  s_bitmap_moon = gbitmap_create_with_resource(RESOURCE_ID_MOON);
  s_bitmap_layer = bitmap_layer_create(g_watch_bound);
  bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bitmap_layer));
}

static void window_unload(Window *window) {
  layer_destroy(s_simple_bg_layer);
  layer_destroy(s_hands_layer);
  text_layer_destroy(s_text_layer);
  text_layer_destroy(s_text_layer2);
  
  gbitmap_destroy(s_bitmap_sun);
  gbitmap_destroy(s_bitmap_moon);
  bitmap_layer_destroy(s_bitmap_layer);
}

static void init() {
  s_window = window_create();
  window_set_click_config_provider(s_window, click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    s_tick_paths[i] = gpath_create(&ANALOG_BG_POINTS[i]);
  }

  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
}

static void deinit() {
  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    gpath_destroy(s_tick_paths[i]);
  }

  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}