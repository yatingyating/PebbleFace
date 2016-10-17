#include <pebble.h>
#include "stepcount.h"
#include "hands.h"
#include "main.h"

Window * step_window;
BitmapLayer * step_bitmap_layer;
TextLayer * step_text_layer;
Layer * step_shadow_layer;
GBitmap * image_bitmap;

static int step_sum = 0;

void disable_step_window() {
  bShowCount = false;
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);  
  window_stack_pop(false);
}

static void step_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Toggle to go back to the main watchface
  disable_step_window();
}

static void step_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, step_select_click_handler);
}

bool isUpdated = false;

void step_window_update(Layer * layer, GContext * ctx) {
  GRect bounds = layer_get_bounds(window_get_root_layer(step_window));
  
  step_sum = get_step_count_today();
  
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
  APP_LOG(APP_LOG_LEVEL_INFO, "sec: %d", tick_time->tm_sec);
  
  float percentage = step_sum / (float)TARGET_STEPS;
  int percentage_int = (int)(percentage * 100 + 0.5);
  int y_start = (int)(percentage * bounds.size.h);
  if (y_start == bounds.size.h) {
    // If we have reached the goal, then show animation
    // (Original intention was to show a GIF, but for some reason pebble had trouble loading them)
    if (!isUpdated) {
      GRect start = bounds;
      GRect end = GRect(0, 10, bounds.size.w, bounds.size.h - 10);
      PropertyAnimation * prop_anim =
        property_animation_create_layer_frame(bitmap_layer_get_layer(step_bitmap_layer), &start, &end);
      Animation * anim = property_animation_get_animation(prop_anim);
      animation_set_curve(anim, AnimationCurveEaseOut);
      animation_set_duration(anim, 100);
      
      Animation *anim_r = animation_clone(anim);
      animation_set_reverse(anim_r, true);
      animation_set_duration(anim_r, 100);
      
      animation_set_delay(anim, 200);
      
      // Create the sequence
      Animation * sequence = animation_sequence_create(anim, anim_r, NULL);
      animation_set_play_count(sequence, ANIMATION_PLAY_COUNT_INFINITE);
  
      // Play the sequence
      animation_schedule(sequence);  
      isUpdated = true;
    }
  } else {
    // Show the shadow corresponding to the completion percentage
    GRect shadow_rect = GRect(0, y_start, bounds.size.w, bounds.size.h - y_start);
    graphics_context_set_fill_color(ctx, GColorDarkGray);
    graphics_fill_rect(ctx, shadow_rect, 0, GCornerNone);
    
    // Show the completion percentage on text layer
    static char labelText[10];
    snprintf(labelText, sizeof(labelText), "%d%%", percentage_int);
    text_layer_set_text(step_text_layer, labelText);
    
    // Adjust the text layer position
    Layer *timeLayer_layer = text_layer_get_layer(step_text_layer);
    layer_set_frame(timeLayer_layer, shadow_rect);
  }
}

void step_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  step_bitmap_layer = bitmap_layer_create(bounds);
  image_bitmap = gbitmap_create_with_resource(RESOURCE_ID_STATIC);
  bitmap_layer_set_alignment(step_bitmap_layer, GAlignTop);
  bitmap_layer_set_bitmap(step_bitmap_layer, image_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(step_bitmap_layer));
  
  step_shadow_layer = layer_create(bounds);
  layer_add_child(window_layer, step_shadow_layer);
  layer_set_update_proc(step_shadow_layer, step_window_update);
  
  step_text_layer = text_layer_create(bounds);
  text_layer_set_text_alignment(step_text_layer, GTextAlignmentCenter);
  text_layer_set_background_color(step_text_layer, GColorClear);
  text_layer_set_text_color(step_text_layer, GColorWhite);
  text_layer_set_font(step_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(step_text_layer));
}

void step_window_unload(Window *window) {
  text_layer_destroy(step_text_layer);
  gbitmap_destroy(image_bitmap);
  
  layer_destroy(step_shadow_layer);
  bitmap_layer_destroy(step_bitmap_layer);
}

void step_window_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(step_shadow_layer);
}

void step_handle_init() {
  step_window = window_create();
  window_set_click_config_provider(step_window, step_click_config_provider);
  window_set_window_handlers(step_window, (WindowHandlers) {
    .load = step_window_load,
    .unload = step_window_unload,
  });

  window_set_background_color(step_window, GColorWhite);
}

void step_handle_deinit() {
  window_destroy(step_window);
}


