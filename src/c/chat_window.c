#include "chat_window.h"
#include "message_bubble.h"
#include "chat_footer.h"
#include "grok_pulse.h"
#include "quick_reply.h"
#include "rsvp_reader.h"

#ifdef PBL_PLATFORM_APLITE
  #define MAX_MESSAGES 4
  #define MESSAGE_TEXT_MAX 256
  // Must fit within AppMessage outbox (512 bytes on Aplite) minus ~64 bytes dict overhead
  #define MESSAGE_BUFFER_SIZE 448
#else
  #define MAX_MESSAGES 4
  #define MESSAGE_TEXT_MAX 512
  // Must fit within AppMessage outbox (2048 bytes) minus ~64 bytes dict overhead
  #define MESSAGE_BUFFER_SIZE 1984
#endif

#define SCROLL_OFFSET 60

// Message data structure
typedef struct {
  char text[MESSAGE_TEXT_MAX];
  bool is_user;
} Message;

// Global state for the chat window
static Window *s_window;
static StatusBarLayer *s_status_bar;
static ScrollLayer *s_scroll_layer;
static Layer *s_content_layer;
static Layer *s_action_button_layer;
static ChatFooter *s_footer;

// Quick reply state
static QuickReply *s_quick_reply = NULL;
static bool s_quick_reply_mode = false;

// RSVP reader state
static RSVPReader *s_rsvp_reader = NULL;
static bool s_rsvp_mode = false;
static RSVPConfig s_rsvp_config;

// RSVP countdown state
static bool s_countdown_active = false;
static int s_countdown_value = 0;
static AppTimer *s_countdown_timer = NULL;
static Layer *s_countdown_layer = NULL;
static TextLayer *s_countdown_text_layer = NULL;

// Message storage (designed for dynamic updates)
static Message s_messages[MAX_MESSAGES];
static int s_message_count = 0;

// Current UI state (bubble instances)
static MessageBubble *s_bubbles[MAX_MESSAGES];
static int s_bubble_count = 0;

static int s_content_width = 0;
static int s_window_height = 0;

// Chat state
static bool s_waiting_for_response = false;

// Forward declarations
static void rebuild_scroll_content(void);
static void up_click_handler(ClickRecognizerRef recognizer, void *context);
static void down_click_handler(ClickRecognizerRef recognizer, void *context);
static void click_config_provider(void *context);
static void send_chat_request(void);
static void clear_conversation(void);
static void shift_messages(void);
static void add_user_message(const char *text);
static void add_assistant_message(const char *text);
static void scroll_to_bottom(void);
static void action_button_update_proc(Layer *layer, GContext *ctx);
static void show_quick_reply(void);
static void hide_quick_reply(void);
static void send_quick_reply(void);
static void show_rsvp_reader(void);
static void hide_rsvp_reader(void);
static void start_rsvp_countdown(void);
static void cancel_rsvp_countdown(void);
static void countdown_timer_callback(void *context);

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Initialize RSVP config with defaults
  s_rsvp_config = rsvp_config_get_default();

  // Set click config provider on window
  window_set_click_config_provider(window, click_config_provider);

  // Create status bar - dark theme
  s_status_bar = status_bar_layer_create();
  status_bar_layer_set_colors(s_status_bar, GColorBlack, GColorWhite);
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));

  // Calculate content area
  s_content_width = bounds.size.w;
  s_window_height = bounds.size.h;
  int status_bar_height = STATUS_BAR_LAYER_HEIGHT;

  // Create scroll layer (below status bar)
  s_scroll_layer = scroll_layer_create(GRect(0, status_bar_height, s_content_width, bounds.size.h - status_bar_height));
  scroll_layer_set_shadow_hidden(s_scroll_layer, true);
  layer_add_child(window_layer, scroll_layer_get_layer(s_scroll_layer));

  // Create content layer (will be resized in rebuild)
  s_content_layer = layer_create(GRect(0, 0, s_content_width, 100));
  scroll_layer_add_child(s_scroll_layer, s_content_layer);

  // Create footer
  s_footer = chat_footer_create(s_content_width);

  // Create action button layer (spans entire window)
  s_action_button_layer = layer_create(bounds);
  layer_set_update_proc(s_action_button_layer, action_button_update_proc);
  layer_add_child(window_layer, s_action_button_layer);

  // Build the UI from message data
  rebuild_scroll_content();

  // Show quick reply selector automatically when window loads
  if (!s_waiting_for_response) {
    show_quick_reply();
  }
}

static void rebuild_scroll_content(void) {
  // Save current scroll position to restore after rebuild
  GPoint saved_offset = scroll_layer_get_content_offset(s_scroll_layer);

  // Destroy old bubbles
  for (int i = 0; i < s_bubble_count; i++) {
    if (s_bubbles[i]) {
      layer_remove_from_parent(message_bubble_get_layer(s_bubbles[i]));
      message_bubble_destroy(s_bubbles[i]);
      s_bubbles[i] = NULL;
    }
  }
  s_bubble_count = 0;

  // Remove footer from content layer
  layer_remove_from_parent(chat_footer_get_layer(s_footer));

  // Create new bubbles from message data
  int y_offset = 0;
  int bubble_max_width = s_content_width;

  for (int i = 0; i < s_message_count; i++) {
    MessageBubble *bubble = message_bubble_create(
      s_messages[i].text,
      s_messages[i].is_user,
      bubble_max_width
    );

    if (bubble) {
      s_bubbles[s_bubble_count++] = bubble;

      // Position bubble
      Layer *bubble_layer = message_bubble_get_layer(bubble);
      GRect frame = layer_get_frame(bubble_layer);
      frame.origin.x = 0;
      frame.origin.y = y_offset;
      layer_set_frame(bubble_layer, frame);
      layer_add_child(s_content_layer, bubble_layer);

      y_offset += message_bubble_get_height(bubble);
    }
  }

  // Always show footer so Grok logo is visible on all screens
  bool show_footer = true;
  
  if (show_footer) {
    // Add top padding only if last message is from user
    bool last_is_user = (s_message_count > 0) && s_messages[s_message_count - 1].is_user;
    if (last_is_user) {
      y_offset += 10;  // Add padding before footer
    }

    int footer_height = chat_footer_get_height(s_footer);
    Layer *footer_layer = chat_footer_get_layer(s_footer);
    GRect footer_frame = layer_get_frame(footer_layer);
    footer_frame.origin.x = 0;
    footer_frame.origin.y = y_offset;
    layer_set_frame(footer_layer, footer_frame);
    layer_add_child(s_content_layer, footer_layer);

    y_offset += footer_height;
  }

  // Update content layer size
  GRect content_frame = layer_get_frame(s_content_layer);
  content_frame.size.h = y_offset;
  layer_set_frame(s_content_layer, content_frame);

  // Update scroll layer content size
  scroll_layer_set_content_size(s_scroll_layer, GSize(s_content_width, y_offset));

  // Restore previous scroll position (prevents jumping during rebuilds)
  scroll_layer_set_content_offset(s_scroll_layer, saved_offset, false);
}

static void clear_conversation(void) {
  // Free all existing bubbles
  for (int i = 0; i < s_bubble_count; i++) {
    if (s_bubbles[i]) {
      layer_remove_from_parent(message_bubble_get_layer(s_bubbles[i]));
      message_bubble_destroy(s_bubbles[i]);
      s_bubbles[i] = NULL;
    }
  }
  s_bubble_count = 0;
  s_message_count = 0;
}

static void shift_messages(void) {
  if (s_message_count == 0) {
    return;
  }

  // Shift all messages one position forward (removing the first/oldest message)
  for (int i = 0; i < s_message_count - 1; i++) {
    s_messages[i] = s_messages[i + 1];
  }

  // Decrement count to free up the last slot
  s_message_count--;
}

static void add_user_message(const char *text) {
  if (s_message_count >= MAX_MESSAGES) {
    // Message array is full, shift to make room
    shift_messages();
  }

  // Add the new message
  snprintf(s_messages[s_message_count].text, sizeof(s_messages[s_message_count].text), "%s", text);
  s_messages[s_message_count].is_user = true;
  s_message_count++;

  // Rebuild the UI to show the new message
  rebuild_scroll_content();
}

static void add_assistant_message(const char *text) {
  if (s_message_count >= MAX_MESSAGES) {
    // Message array is full, shift to make room
    shift_messages();
  }

  // Add empty or initial assistant message
  snprintf(s_messages[s_message_count].text, sizeof(s_messages[s_message_count].text), "%s", text);
  s_messages[s_message_count].is_user = false;
  s_message_count++;

  // Rebuild UI
  rebuild_scroll_content();
}

static void scroll_to_bottom(void) {
  GRect content_bounds = layer_get_bounds(s_content_layer);
  GRect scroll_bounds = layer_get_bounds(scroll_layer_get_layer(s_scroll_layer));

  int max_offset = content_bounds.size.h - scroll_bounds.size.h;
  if (max_offset < 0) {
    max_offset = 0;
  }

  scroll_layer_set_content_offset(s_scroll_layer, GPoint(0, -max_offset), true);
}

static void send_chat_request(void) {
  // Encode all messages into format: "[U]msg1[A]msg2[U]msg3..."
  static char encoded_buffer[MESSAGE_BUFFER_SIZE];
  encoded_buffer[0] = '\0';

  for (int i = 0; i < s_message_count; i++) {
    const char *prefix = s_messages[i].is_user ? "[U]" : "[A]";
    size_t current_len = strlen(encoded_buffer);
    size_t available = MESSAGE_BUFFER_SIZE - current_len - 1;

    // Add prefix
    strncat(encoded_buffer, prefix, available);
    current_len = strlen(encoded_buffer);
    available = MESSAGE_BUFFER_SIZE - current_len - 1;

    // Add message text
    strncat(encoded_buffer, s_messages[i].text, available);
  }

  // Send via AppMessage
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);

  if (result == APP_MSG_OK) {
    dict_write_cstring(iter, MESSAGE_KEY_REQUEST_CHAT, encoded_buffer);
    result = app_message_outbox_send();

    if (result == APP_MSG_OK) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Sent REQUEST_CHAT: %d bytes", (int)strlen(encoded_buffer));
      s_waiting_for_response = true;
      chat_window_set_footer_animating(true);
    } else {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to send REQUEST_CHAT: %d", (int)result);
    }
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to begin outbox: %d", (int)result);
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Cancel countdown on any button press
  if (s_countdown_active) {
    cancel_rsvp_countdown();
    return;
  }
  
  if (s_rsvp_mode && s_rsvp_reader) {
    // In RSVP mode: skip backward
    rsvp_reader_skip_back(s_rsvp_reader);
  } else if (s_quick_reply_mode && s_quick_reply) {
    // In quick reply mode: cycle to previous option
    quick_reply_prev(s_quick_reply);
  } else {
    // Normal mode: scroll up
    GPoint offset = scroll_layer_get_content_offset(s_scroll_layer);
    offset.y += SCROLL_OFFSET;
    scroll_layer_set_content_offset(s_scroll_layer, offset, true);
  }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Cancel countdown on any button press
  if (s_countdown_active) {
    cancel_rsvp_countdown();
    return;
  }
  
  if (s_rsvp_mode && s_rsvp_reader) {
    // In RSVP mode: skip forward
    rsvp_reader_skip_forward(s_rsvp_reader);
  } else if (s_quick_reply_mode && s_quick_reply) {
    // In quick reply mode: cycle to next option
    quick_reply_next(s_quick_reply);
  } else {
    // Normal mode: scroll down
    GPoint offset = scroll_layer_get_content_offset(s_scroll_layer);
    offset.y -= SCROLL_OFFSET;
    scroll_layer_set_content_offset(s_scroll_layer, offset, true);
  }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Cancel countdown on any button press
  if (s_countdown_active) {
    cancel_rsvp_countdown();
    return;
  }
  
  // Handle RSVP mode: toggle pause
  if (s_rsvp_mode && s_rsvp_reader) {
    rsvp_reader_toggle_pause(s_rsvp_reader);
    return;
  }

  // Don't allow input if waiting for response
  if (s_waiting_for_response) {
    return;
  }

  if (s_quick_reply_mode) {
    // In quick reply mode: send selected message
    send_quick_reply();
  } else {
    // Normal mode: show quick reply selector
    show_quick_reply();
  }
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Cancel countdown on any button press
  if (s_countdown_active) {
    cancel_rsvp_countdown();
    return;
  }
  
  if (s_rsvp_mode) {
    // In RSVP mode: exit RSVP
    hide_rsvp_reader();
  } else if (s_quick_reply_mode) {
    // In quick reply mode: exit without sending
    hide_quick_reply();
  } else {
    // Normal mode: exit app
    window_stack_pop(true);
  }
}

// Action button indicator on the right side
static void action_button_update_proc(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);

  // Button radius (13 for rectangular displays, 12 for round)
  const int radius = PBL_IF_ROUND_ELSE(12, 13);

  // Create button rect and align it to the right side
  GRect button_rect = GRect(0, 0, radius * 2, radius * 2);
  grect_align(&button_rect, &bounds, GAlignRight, false);

  // Offset the button halfway off-screen
  button_rect.origin.x += radius;

  // Further offset on a per-content-size basis
  button_rect.origin.x += PBL_IF_ROUND_ELSE(1, 8);

  // Draw filled circle - electric blue on color, white on B&W for dark theme
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorVividCerulean, GColorWhite));
  graphics_fill_radial(ctx, button_rect, GOvalScaleModeFitCircle, radius, 0, TRIG_MAX_ANGLE);

  // Draw 1px outer outline (B/W displays only)
#ifdef PBL_BW
  GRect outline_rect = grect_inset(button_rect, GEdgeInsets(-1));
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_arc(ctx, outline_rect, GOvalScaleModeFitCircle, 0, TRIG_MAX_ANGLE);
#endif
}

static void click_config_provider(void *context) {
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
}

static void show_quick_reply(void) {
  if (s_quick_reply_mode) {
    return;  // Already showing
  }

  // Calculate fullscreen dimensions (below status bar)
  int status_bar_height = STATUS_BAR_LAYER_HEIGHT;
  int qr_height = s_window_height - status_bar_height;

  // Create fullscreen quick reply overlay
  s_quick_reply = quick_reply_create_fullscreen(s_content_width, qr_height);
  if (!s_quick_reply) {
    return;
  }

  // Position below status bar, filling the rest of the screen
  Layer *qr_layer = quick_reply_get_layer(s_quick_reply);
  GRect qr_frame = layer_get_frame(qr_layer);
  qr_frame.origin.x = 0;
  qr_frame.origin.y = status_bar_height;
  layer_set_frame(qr_layer, qr_frame);

  // Add to window (above scroll layer)
  Layer *window_layer = window_get_root_layer(s_window);
  layer_add_child(window_layer, qr_layer);

  s_quick_reply_mode = true;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Quick reply mode enabled");
}

static void hide_quick_reply(void) {
  if (!s_quick_reply_mode || !s_quick_reply) {
    return;
  }

  // Remove and destroy quick reply
  layer_remove_from_parent(quick_reply_get_layer(s_quick_reply));
  quick_reply_destroy(s_quick_reply);
  s_quick_reply = NULL;

  s_quick_reply_mode = false;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Quick reply mode disabled");
}

static void send_quick_reply(void) {
  if (!s_quick_reply_mode || !s_quick_reply) {
    return;
  }

  // Get selected message
  const char *message = quick_reply_get_selected(s_quick_reply);
  if (!message) {
    hide_quick_reply();
    return;
  }

  // Hide the quick reply UI first
  hide_quick_reply();

  // Clear previous conversation for fresh single-turn start
  clear_conversation();

  // Add as user message and send
  add_user_message(message);
  scroll_to_bottom();
  send_chat_request();
}

static void show_rsvp_reader(void) {
  if (s_rsvp_mode) {
    return;  // Already showing
  }
  
  // Find the most recent assistant message
  const char *assistant_text = NULL;
  for (int i = s_message_count - 1; i >= 0; i--) {
    if (!s_messages[i].is_user) {
      assistant_text = s_messages[i].text;
      break;
    }
  }
  
  if (!assistant_text || strlen(assistant_text) == 0) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "No assistant message to read with RSVP");
    return;
  }
  
  // Hide quick reply if showing
  if (s_quick_reply_mode) {
    hide_quick_reply();
  }
  
  // Calculate fullscreen dimensions (below status bar)
  int status_bar_height = STATUS_BAR_LAYER_HEIGHT;
  int rsvp_height = s_window_height - status_bar_height;
  
  // Create RSVP reader overlay
  s_rsvp_reader = rsvp_reader_create(s_content_width, rsvp_height, assistant_text, &s_rsvp_config);
  if (!s_rsvp_reader) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to create RSVP reader");
    return;
  }
  
  // Position below status bar
  Layer *rsvp_layer = rsvp_reader_get_layer(s_rsvp_reader);
  GRect rsvp_frame = layer_get_frame(rsvp_layer);
  rsvp_frame.origin.x = 0;
  rsvp_frame.origin.y = status_bar_height;
  layer_set_frame(rsvp_layer, rsvp_frame);
  
  // Add to window (above scroll layer)
  Layer *window_layer = window_get_root_layer(s_window);
  layer_add_child(window_layer, rsvp_layer);
  
  s_rsvp_mode = true;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "RSVP reader mode enabled");
}

static void hide_rsvp_reader(void) {
  if (!s_rsvp_mode || !s_rsvp_reader) {
    return;
  }
  
  // Remove and destroy RSVP reader
  layer_remove_from_parent(rsvp_reader_get_layer(s_rsvp_reader));
  rsvp_reader_destroy(s_rsvp_reader);
  s_rsvp_reader = NULL;
  
  s_rsvp_mode = false;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "RSVP reader mode disabled");
}

// --- RSVP Countdown Functions ---

static void countdown_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  
  // Fill with dark background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
}

static void countdown_timer_callback(void *context) {
  s_countdown_timer = NULL;
  
  if (!s_countdown_active) {
    return;
  }
  
  s_countdown_value--;
  
  if (s_countdown_value <= 0) {
    // Countdown finished - launch RSVP reader
    cancel_rsvp_countdown();
    show_rsvp_reader();
  } else {
    // Update display and schedule next tick
    static char countdown_text[4];
    snprintf(countdown_text, sizeof(countdown_text), "%d", s_countdown_value);
    text_layer_set_text(s_countdown_text_layer, countdown_text);
    layer_mark_dirty(s_countdown_layer);
    
    // Schedule next tick (1 second)
    s_countdown_timer = app_timer_register(1000, countdown_timer_callback, NULL);
  }
}

static void start_rsvp_countdown(void) {
  if (s_countdown_active || s_rsvp_mode || s_quick_reply_mode) {
    return;
  }
  
  // Check if there's an assistant message to read
  bool has_assistant_msg = false;
  for (int i = s_message_count - 1; i >= 0; i--) {
    if (!s_messages[i].is_user) {
      has_assistant_msg = true;
      break;
    }
  }
  
  if (!has_assistant_msg) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "No assistant message for RSVP countdown");
    return;
  }
  
  // Hide quick reply if showing
  if (s_quick_reply_mode) {
    hide_quick_reply();
  }
  
  // Calculate fullscreen dimensions (below status bar)
  int status_bar_height = STATUS_BAR_LAYER_HEIGHT;
  int countdown_height = s_window_height - status_bar_height;
  
  // Create countdown overlay layer
  s_countdown_layer = layer_create(GRect(0, status_bar_height, s_content_width, countdown_height));
  layer_set_update_proc(s_countdown_layer, countdown_layer_update_proc);
  
  // Create large centered text for countdown number
  int text_height = 60;
  int text_y = (countdown_height - text_height) / 2;
  s_countdown_text_layer = text_layer_create(GRect(0, text_y, s_content_width, text_height));
  text_layer_set_font(s_countdown_text_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
  text_layer_set_text_alignment(s_countdown_text_layer, GTextAlignmentCenter);
  text_layer_set_text_color(s_countdown_text_layer, GColorWhite);
  text_layer_set_background_color(s_countdown_text_layer, GColorClear);
  text_layer_set_text(s_countdown_text_layer, "3");
  layer_add_child(s_countdown_layer, text_layer_get_layer(s_countdown_text_layer));
  
  // Add to window
  Layer *window_layer = window_get_root_layer(s_window);
  layer_add_child(window_layer, s_countdown_layer);
  
  // Start countdown
  s_countdown_active = true;
  s_countdown_value = 3;
  
  // Schedule first tick (1 second)
  s_countdown_timer = app_timer_register(1000, countdown_timer_callback, NULL);
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "RSVP countdown started");
}

static void cancel_rsvp_countdown(void) {
  if (!s_countdown_active) {
    return;
  }
  
  // Cancel timer
  if (s_countdown_timer) {
    app_timer_cancel(s_countdown_timer);
    s_countdown_timer = NULL;
  }
  
  // Destroy countdown UI
  if (s_countdown_text_layer) {
    text_layer_destroy(s_countdown_text_layer);
    s_countdown_text_layer = NULL;
  }
  
  if (s_countdown_layer) {
    layer_remove_from_parent(s_countdown_layer);
    layer_destroy(s_countdown_layer);
    s_countdown_layer = NULL;
  }
  
  s_countdown_active = false;
  s_countdown_value = 0;
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "RSVP countdown cancelled");
}

static void window_unload(Window *window) {
  // Clean up countdown if active
  if (s_countdown_active) {
    cancel_rsvp_countdown();
  }
  
  // Clean up RSVP reader if showing
  if (s_rsvp_reader) {
    rsvp_reader_destroy(s_rsvp_reader);
    s_rsvp_reader = NULL;
    s_rsvp_mode = false;
  }
  
  // Clean up quick reply if showing
  if (s_quick_reply) {
    quick_reply_destroy(s_quick_reply);
    s_quick_reply = NULL;
    s_quick_reply_mode = false;
  }

  // Destroy all bubbles
  for (int i = 0; i < s_bubble_count; i++) {
    if (s_bubbles[i]) {
      message_bubble_destroy(s_bubbles[i]);
      s_bubbles[i] = NULL;
    }
  }
  s_bubble_count = 0;

  // Reset message history
  s_message_count = 0;

  // Destroy footer
  if (s_footer) {
    chat_footer_destroy(s_footer);
    s_footer = NULL;
  }

  // Destroy UI components
  if (s_content_layer) {
    layer_destroy(s_content_layer);
  }

  if (s_scroll_layer) {
    scroll_layer_destroy(s_scroll_layer);
  }

  if (s_action_button_layer) {
    layer_destroy(s_action_button_layer);
  }

  if (s_status_bar) {
    status_bar_layer_destroy(s_status_bar);
  }
}

void chat_window_handle_inbox(DictionaryIterator *iterator) {
  // Handle incoming messages from JS
  Tuple *response_text_tuple = dict_find(iterator, MESSAGE_KEY_RESPONSE_TEXT);
  Tuple *response_end_tuple = dict_find(iterator, MESSAGE_KEY_RESPONSE_END);
  Tuple *phone_msg_tuple = dict_find(iterator, MESSAGE_KEY_PHONE_MESSAGE);
  
  // Handle RSVP configuration updates
  Tuple *rsvp_tuple;
  
  rsvp_tuple = dict_find(iterator, MESSAGE_KEY_RSVP_ENABLED);
  if (rsvp_tuple) {
    s_rsvp_config.enabled = (rsvp_tuple->value->int32 == 1);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "RSVP enabled: %d", s_rsvp_config.enabled);
  }
  
  rsvp_tuple = dict_find(iterator, MESSAGE_KEY_RSVP_WPM);
  if (rsvp_tuple) {
    s_rsvp_config.wpm = (uint16_t)rsvp_tuple->value->int32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "RSVP WPM: %d", s_rsvp_config.wpm);
  }
  
  rsvp_tuple = dict_find(iterator, MESSAGE_KEY_RSVP_WORDS_PER_CHUNK);
  if (rsvp_tuple) {
    s_rsvp_config.words_per_chunk = (uint8_t)rsvp_tuple->value->int32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "RSVP words per chunk: %d", s_rsvp_config.words_per_chunk);
  }
  
  rsvp_tuple = dict_find(iterator, MESSAGE_KEY_RSVP_PAUSE_ON_PUNCTUATION);
  if (rsvp_tuple) {
    s_rsvp_config.pause_on_punctuation = (rsvp_tuple->value->int32 == 1);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "RSVP pause on punctuation: %d", s_rsvp_config.pause_on_punctuation);
  }
  
  rsvp_tuple = dict_find(iterator, MESSAGE_KEY_RSVP_SKIP_WORDS);
  if (rsvp_tuple) {
    s_rsvp_config.skip_words = (uint8_t)rsvp_tuple->value->int32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "RSVP skip words: %d", s_rsvp_config.skip_words);
  }
  
  rsvp_tuple = dict_find(iterator, MESSAGE_KEY_RSVP_SHOW_PROGRESS);
  if (rsvp_tuple) {
    s_rsvp_config.show_progress = (rsvp_tuple->value->int32 == 1);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "RSVP show progress: %d", s_rsvp_config.show_progress);
  }

  // Handle phone message (typed on phone, sent to watch)
  if (phone_msg_tuple) {
    const char *text = phone_msg_tuple->value->cstring;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Received PHONE_MESSAGE: %s", text);

    // Cancel active RSVP before clearing
    if (s_countdown_active) {
      cancel_rsvp_countdown();
    }
    if (s_rsvp_mode) {
      hide_rsvp_reader();
    }

    // Hide quick reply if showing
    if (s_quick_reply_mode) {
      hide_quick_reply();
    }

    // Clear previous conversation for fresh single-turn start
    clear_conversation();

    // Add as user message and send to Grok
    add_user_message(text);
    scroll_to_bottom();
    send_chat_request();
  }

  if (response_text_tuple) {
    // Received complete response text
    const char *text = response_text_tuple->value->cstring;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Received RESPONSE_TEXT: %s", text);

    // Add as new assistant message
    add_assistant_message(text);
  }

  if (response_end_tuple) {
    // Response complete - unlock UI
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Received RESPONSE_END");
    s_waiting_for_response = false;
    chat_window_set_footer_animating(false);
    
    // Start RSVP countdown if enabled and not already in RSVP mode
    if (s_rsvp_config.enabled && !s_rsvp_mode && !s_countdown_active) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Starting RSVP countdown after response");
      start_rsvp_countdown();
    }
  }
}

Window* chat_window_create(void) {
  s_window = window_create();
  // Dark theme background
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  return s_window;
}

void chat_window_destroy(Window *window) {
  if (window) {
    window_destroy(window);
  }
}

void chat_window_set_footer_animating(bool animating) {
  if (!s_footer) {
    return;
  }

  if (animating) {
    // Footer is always visible, just scroll and animate
    scroll_to_bottom();
    chat_footer_start_animation(s_footer);
  } else {
    chat_footer_stop_animation(s_footer);
  }
}

void chat_window_refresh_quick_reply(void) {
  if (s_quick_reply_mode && s_quick_reply) {
    quick_reply_refresh(s_quick_reply);
  }
}
