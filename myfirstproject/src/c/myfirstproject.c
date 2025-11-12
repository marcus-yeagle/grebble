// ...existing code...
#include <pebble.h>

#define SETTINGS_KEY 1

static Window *s_window;
static Layer *s_canvas;
static GPoint s_center;
static int16_t s_radius;
static bool s_show_numbers = true;

// Weather data
static char s_weather_temp[8] = "--°";
static char s_weather_condition[32] = "☁";

static void prv_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Background
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Center and radius (recompute in case of orientation change)
  s_center = GPoint(bounds.size.w / 2, bounds.size.h / 2);
  s_radius = ((bounds.size.w < bounds.size.h ? bounds.size.w : bounds.size.h) / 2) - 4;

  // Draw rectangular hour indicators at edges of display
  graphics_context_set_fill_color(ctx, GColorBlack);
  int16_t indicator_width = 4;
  int16_t indicator_height = 2;
  int16_t margin = 2;
  
  // Top edge: 11, 12, 1 (left to right, evenly spaced)
  int16_t top_spacing = bounds.size.w / 4;
  // 11 o'clock
  graphics_fill_rect(ctx, GRect(top_spacing - indicator_width / 2, margin, indicator_width, indicator_height), 0, GCornerNone);
  // 12 o'clock (center)
  graphics_fill_rect(ctx, GRect(s_center.x - indicator_width / 2, margin, indicator_width, indicator_height), 0, GCornerNone);
  // 1 o'clock
  graphics_fill_rect(ctx, GRect(top_spacing * 3 - indicator_width / 2, margin, indicator_width, indicator_height), 0, GCornerNone);
  
  // Right edge: 2, 3, 4 (top to bottom, evenly spaced)
  int16_t right_spacing = bounds.size.h / 4;
  // 2 o'clock
  graphics_fill_rect(ctx, GRect(bounds.size.w - margin - indicator_width, right_spacing - indicator_height / 2, indicator_width, indicator_height), 0, GCornerNone);
  // 3 o'clock (center)
  graphics_fill_rect(ctx, GRect(bounds.size.w - margin - indicator_width, s_center.y - indicator_height / 2, indicator_width, indicator_height), 0, GCornerNone);
  // 4 o'clock
  graphics_fill_rect(ctx, GRect(bounds.size.w - margin - indicator_width, right_spacing * 3 - indicator_height / 2, indicator_width, indicator_height), 0, GCornerNone);
  
  // Bottom edge: 5, 6, 7 (right to left, evenly spaced)
  // 5 o'clock
  graphics_fill_rect(ctx, GRect(top_spacing * 3 - indicator_width / 2, bounds.size.h - margin - indicator_height, indicator_width, indicator_height), 0, GCornerNone);
  // 6 o'clock (center)
  graphics_fill_rect(ctx, GRect(s_center.x - indicator_width / 2, bounds.size.h - margin - indicator_height, indicator_width, indicator_height), 0, GCornerNone);
  // 7 o'clock
  graphics_fill_rect(ctx, GRect(top_spacing - indicator_width / 2, bounds.size.h - margin - indicator_height, indicator_width, indicator_height), 0, GCornerNone);
  
  // Left edge: 8, 9, 10 (bottom to top, evenly spaced)
  // 8 o'clock
  graphics_fill_rect(ctx, GRect(margin, right_spacing * 3 - indicator_height / 2, indicator_width, indicator_height), 0, GCornerNone);
  // 9 o'clock (center)
  graphics_fill_rect(ctx, GRect(margin, s_center.y - indicator_height / 2, indicator_width, indicator_height), 0, GCornerNone);
  // 10 o'clock
  graphics_fill_rect(ctx, GRect(margin, right_spacing - indicator_height / 2, indicator_width, indicator_height), 0, GCornerNone);

  // Get current time
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  // Draw date vertically on the right side
  static char day_buffer[4];
  static char date_buffer[3];
  static char month_buffer[4];
  
  strftime(day_buffer, sizeof(day_buffer), "%a", t);  // Thu
  strftime(date_buffer, sizeof(date_buffer), "%e", t);  // 30
  strftime(month_buffer, sizeof(month_buffer), "%b", t); // Oct
  
  graphics_context_set_text_color(ctx, GColorBlack);
  GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  
  int16_t date_x = bounds.size.w - 28;
  int16_t start_y = s_center.y - 30;
  
  // Draw day of week
  graphics_draw_text(ctx, day_buffer, font,
                     GRect(date_x - 20, start_y, 40, 22),
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  
  // Draw date
  graphics_draw_text(ctx, date_buffer, font,
                     GRect(date_x - 20, start_y + 20, 40, 22),
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  
  // Draw month
  graphics_draw_text(ctx, month_buffer, font,
                     GRect(date_x - 20, start_y + 40, 40, 22),
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

  // Draw weather on the left side
  int16_t weather_x = 28;
  int16_t weather_start_y = s_center.y - 10;
  
  GFont temp_font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  
  // Draw temperature (icon removed to avoid blank box issue)
  graphics_draw_text(ctx, s_weather_temp, temp_font,
                     GRect(weather_x - 5, weather_start_y, 40, 22),
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

  // Calculate angles
  int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
  int32_t minute_angle = TRIG_MAX_ANGLE * ((t->tm_min * 60) + t->tm_sec) / 3600;
  int32_t hour_angle = TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 3600) + (t->tm_min * 60) + t->tm_sec) / 43200;

  // Second hand
  int16_t second_length = s_radius - 10;
  GPoint second_end = {
    .x = (int16_t)(s_center.x + (sin_lookup(second_angle) * second_length / TRIG_MAX_RATIO)),
    .y = (int16_t)(s_center.y - (cos_lookup(second_angle) * second_length / TRIG_MAX_RATIO))
  };
  graphics_context_set_stroke_color(ctx, GColorRed);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, s_center, second_end);

  // Minute hand
  int16_t minute_length = s_radius - 5;
  GPoint minute_end = {
    .x = (int16_t)(s_center.x + (sin_lookup(minute_angle) * minute_length / TRIG_MAX_RATIO)),
    .y = (int16_t)(s_center.y - (cos_lookup(minute_angle) * minute_length / TRIG_MAX_RATIO))
  };
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 5);
  graphics_draw_line(ctx, s_center, minute_end);

  // Hour hand
  int16_t hour_length = s_radius - 30;
  GPoint hour_end = {
    .x = (int16_t)(s_center.x + (sin_lookup(hour_angle) * hour_length / TRIG_MAX_RATIO)),
    .y = (int16_t)(s_center.y - (cos_lookup(hour_angle) * hour_length / TRIG_MAX_RATIO))
  };
  graphics_context_set_stroke_width(ctx, 3);
  graphics_draw_line(ctx, s_center, hour_end);

  // Center dot
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, s_center, 3);
}

static void prv_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_canvas);
}

static void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {
  // Read boolean preferences
  Tuple *show_numbers_t = dict_find(iter, MESSAGE_KEY_ShowNumbers);
  if (show_numbers_t) {
    s_show_numbers = show_numbers_t->value->int32 == 1;
    persist_write_bool(SETTINGS_KEY, s_show_numbers);
    layer_mark_dirty(s_canvas);
  }
  
  // Read weather data
  Tuple *temp_tuple = dict_find(iter, MESSAGE_KEY_Temperature);
  Tuple *condition_tuple = dict_find(iter, MESSAGE_KEY_Conditions);
  
  if (temp_tuple) {
    snprintf(s_weather_temp, sizeof(s_weather_temp), "%d°", (int)temp_tuple->value->int32);
    layer_mark_dirty(s_canvas);
  }
  
  if (condition_tuple) {
    snprintf(s_weather_condition, sizeof(s_weather_condition), "%s", condition_tuple->value->cstring);
    layer_mark_dirty(s_canvas);
  }
}

static void prv_load_settings() {
  // Load stored settings or use defaults
  s_show_numbers = persist_exists(SETTINGS_KEY) ? persist_read_bool(SETTINGS_KEY) : true;
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_canvas = layer_create(bounds);
  layer_set_update_proc(s_canvas, prv_update_proc);
  layer_add_child(window_layer, s_canvas);
}

static void prv_window_unload(Window *window) {
  if (s_canvas) {
    layer_destroy(s_canvas);
    s_canvas = NULL;
  }
}

static void prv_init(void) {
  // Load settings
  prv_load_settings();

  s_window = window_create();
  window_set_background_color(s_window, GColorWhite);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });

  // Register AppMessage handlers
  app_message_register_inbox_received(prv_inbox_received_handler);
  app_message_open(128, 128);

  const bool animated = true;
  window_stack_push(s_window, animated);

  // Update once immediately then every second for smooth movement
  tick_timer_service_subscribe(SECOND_UNIT, prv_tick_handler);
  layer_mark_dirty(window_get_root_layer(s_window));
}

static void prv_deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Analog watchface initialized: %p", s_window);

  app_event_loop();
  prv_deinit();
}