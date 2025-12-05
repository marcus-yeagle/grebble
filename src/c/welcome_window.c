#include "welcome_window.h"

// Forward declaration for transition function (defined in grok-for-pebble.c)
extern void show_chat_window_from_welcome(void);

// Global state for the welcome window
static Window *s_window;
static TextLayer *s_text_layer;
static ActionBarLayer *s_action_bar;
static GBitmap *s_action_icon_select;

// Forward declarations
static void window_load(Window *window);
static void window_unload(Window *window);
static void select_click_handler(ClickRecognizerRef recognizer, void *context);
static void click_config_provider(void *context);

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create action bar - dark theme
  s_action_bar = action_bar_layer_create();
  action_bar_layer_set_background_color(s_action_bar, PBL_IF_COLOR_ELSE(GColorVividCerulean, GColorWhite));
  action_bar_layer_add_to_window(s_action_bar, window);
  action_bar_layer_set_click_config_provider(s_action_bar, click_config_provider);

  // Load and set center icon (microphone/dictation)
  s_action_icon_select = gbitmap_create_with_resource(RESOURCE_ID_ACTION_ICON_DICTATION);
  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_action_icon_select);

  // Calculate available width (accounting for action bar)
  int content_width = bounds.size.w - ACTION_BAR_WIDTH;
  int text_margin = 10;

  // Create text layer filling the entire screen height
  s_text_layer = text_layer_create(
    GRect(text_margin, 0, content_width - text_margin * 2, bounds.size.h)
  );
  text_layer_set_text(s_text_layer, "What do you\nwant to know?");
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_text_layer, GColorClear);
  text_layer_set_overflow_mode(s_text_layer, GTextOverflowModeWordWrap);
  // Dark theme: white text
  text_layer_set_text_color(s_text_layer, GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer));
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Transition to chat window
  show_chat_window_from_welcome();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void window_unload(Window *window) {
  // Destroy action bar and icon
  if (s_action_icon_select) {
    gbitmap_destroy(s_action_icon_select);
    s_action_icon_select = NULL;
  }
  if (s_action_bar) {
    action_bar_layer_destroy(s_action_bar);
    s_action_bar = NULL;
  }

  // Destroy text layer
  if (s_text_layer) {
    text_layer_destroy(s_text_layer);
    s_text_layer = NULL;
  }
}

Window *welcome_window_create(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  // Dark theme background
  window_set_background_color(s_window, GColorBlack);
  return s_window;
}

void welcome_window_destroy(Window *window) {
  if (s_window) {
    window_destroy(s_window);
    s_window = NULL;
  }
}

