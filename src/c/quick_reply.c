#include "quick_reply.h"
#include <string.h>

#define QUICK_REPLY_PADDING 10
#define QUICK_REPLY_TEXT_FONT FONT_KEY_GOTHIC_18_BOLD
#define QUICK_REPLY_HINT_FONT FONT_KEY_GOTHIC_14
#define QUICK_REPLY_ARROW_WIDTH 20

// Default quick replies
static const char* s_default_prompts[] = {
  "Hello",
  "Any news about AI?",
  "Tell me a joke",
  "Thanks!",
  "Goodbye"
};

// Custom prompts storage (configurable from phone settings)
static char s_custom_prompts[NUM_CANNED_PROMPTS][MAX_PROMPT_LENGTH];
static bool s_prompts_initialized = false;
static bool s_prompts_ready = false;  // True when phone has synced prompts

struct QuickReply {
  Layer *layer;
  TextLayer *text_layer;
  TextLayer *hint_layer;
  int width;
  int height;
  int current_index;
};

static void quick_reply_update_proc(Layer *layer, GContext *ctx) {
  QuickReply *qr = (QuickReply *)layer_get_data(layer);
  if (!qr) return;
  
  GRect bounds = layer_get_bounds(layer);
  
  // Fill with dark background (full screen)
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  
  // Draw up/down navigation arrows centered horizontally
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite));
  graphics_context_set_stroke_width(ctx, 2);
  
  int center_x = bounds.size.w / 2;
  
  // Up arrow: ^
  int up_y = QUICK_REPLY_PADDING + 10;
  graphics_draw_line(ctx, GPoint(center_x - 10, up_y + 8), GPoint(center_x, up_y));
  graphics_draw_line(ctx, GPoint(center_x, up_y), GPoint(center_x + 10, up_y + 8));
  
  // Down arrow: v
  int down_y = bounds.size.h - QUICK_REPLY_PADDING - 10;
  graphics_draw_line(ctx, GPoint(center_x - 10, down_y - 8), GPoint(center_x, down_y));
  graphics_draw_line(ctx, GPoint(center_x, down_y), GPoint(center_x + 10, down_y - 8));
}

// Get the prompt at the given index (uses custom if set, otherwise default)
static const char* get_prompt_at_index(int index) {
  if (index < 0 || index >= NUM_CANNED_PROMPTS) return NULL;
  
  // Use custom prompt if it's been set (non-empty)
  if (s_prompts_initialized && s_custom_prompts[index][0] != '\0') {
    return s_custom_prompts[index];
  }
  
  // Fall back to default
  return s_default_prompts[index];
}

static void update_text_display(QuickReply *qr) {
  if (!qr || !qr->text_layer) return;
  
  // Show loading state if prompts haven't been synced from phone yet
  if (!s_prompts_ready) {
    text_layer_set_text(qr->text_layer, "Loading...");
    return;
  }
  
  const char *prompt = get_prompt_at_index(qr->current_index);
  text_layer_set_text(qr->text_layer, prompt ? prompt : "");
}

QuickReply* quick_reply_create_fullscreen(int width, int height) {
  QuickReply *qr = malloc(sizeof(QuickReply));
  if (!qr) return NULL;
  
  qr->width = width;
  qr->height = height;
  qr->current_index = 0;
  qr->hint_layer = NULL;  // Not used in fullscreen mode
  
  // Create container layer with data pointer
  qr->layer = layer_create_with_data(GRect(0, 0, width, height), sizeof(QuickReply*));
  QuickReply **layer_data = (QuickReply **)layer_get_data(qr->layer);
  *layer_data = qr;
  layer_set_update_proc(qr->layer, quick_reply_update_proc);
  
  // Calculate text area - leave room for up/down arrows
  int arrow_margin = 30;  // Space for arrows at top and bottom
  int text_margin = QUICK_REPLY_PADDING;
  int text_width = width - (text_margin * 2);
  int text_height = height - (arrow_margin * 2);
  int text_y = arrow_margin;
  
  // Create text layer for current selection (left aligned, with word wrap)
  qr->text_layer = text_layer_create(GRect(text_margin, text_y, text_width, text_height));
  text_layer_set_font(qr->text_layer, fonts_get_system_font(QUICK_REPLY_TEXT_FONT));
  text_layer_set_text_alignment(qr->text_layer, GTextAlignmentLeft);
  text_layer_set_text_color(qr->text_layer, GColorWhite);
  text_layer_set_background_color(qr->text_layer, GColorClear);
  text_layer_set_overflow_mode(qr->text_layer, GTextOverflowModeWordWrap);
  layer_add_child(qr->layer, text_layer_get_layer(qr->text_layer));
  
  // Set initial text
  update_text_display(qr);
  
  return qr;
}

QuickReply* quick_reply_create(int width) {
  // Default to a reasonable height for backwards compatibility
  return quick_reply_create_fullscreen(width, 120);
}

void quick_reply_destroy(QuickReply *qr) {
  if (!qr) return;
  
  if (qr->hint_layer) {
    text_layer_destroy(qr->hint_layer);
    qr->hint_layer = NULL;
  }
  
  if (qr->text_layer) {
    text_layer_destroy(qr->text_layer);
    qr->text_layer = NULL;
  }
  
  if (qr->layer) {
    layer_destroy(qr->layer);
    qr->layer = NULL;
  }
  
  free(qr);
}

Layer* quick_reply_get_layer(QuickReply *qr) {
  return qr ? qr->layer : NULL;
}

int quick_reply_get_height(QuickReply *qr) {
  return qr ? qr->height : 0;
}

void quick_reply_next(QuickReply *qr) {
  if (!qr) return;
  
  qr->current_index++;
  if (qr->current_index >= NUM_CANNED_PROMPTS) {
    qr->current_index = 0;
  }
  
  update_text_display(qr);
  layer_mark_dirty(qr->layer);
}

void quick_reply_prev(QuickReply *qr) {
  if (!qr) return;
  
  qr->current_index--;
  if (qr->current_index < 0) {
    qr->current_index = NUM_CANNED_PROMPTS - 1;
  }
  
  update_text_display(qr);
  layer_mark_dirty(qr->layer);
}

const char* quick_reply_get_selected(QuickReply *qr) {
  if (!qr) return NULL;
  return get_prompt_at_index(qr->current_index);
}

int quick_reply_get_index(QuickReply *qr) {
  return qr ? qr->current_index : 0;
}

int quick_reply_get_count(QuickReply *qr) {
  return NUM_CANNED_PROMPTS;
}

void quick_reply_init(void) {
  // Initialize custom prompts with empty strings
  for (int i = 0; i < NUM_CANNED_PROMPTS; i++) {
    s_custom_prompts[i][0] = '\0';
  }
  s_prompts_initialized = true;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Quick reply system initialized");
}

void quick_reply_set_prompt(int index, const char *prompt) {
  if (index < 0 || index >= NUM_CANNED_PROMPTS) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Invalid prompt index: %d", index);
    return;
  }
  
  if (!prompt || prompt[0] == '\0') {
    // Empty prompt - clear to use default
    s_custom_prompts[index][0] = '\0';
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Cleared custom prompt %d, using default", index);
  } else {
    // Copy prompt, ensuring null termination
    strncpy(s_custom_prompts[index], prompt, MAX_PROMPT_LENGTH - 1);
    s_custom_prompts[index][MAX_PROMPT_LENGTH - 1] = '\0';
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Set custom prompt %d: %s", index, s_custom_prompts[index]);
  }
}

const char* quick_reply_get_prompt(int index) {
  return get_prompt_at_index(index);
}

void quick_reply_set_ready(bool ready) {
  s_prompts_ready = ready;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Quick reply ready state: %s", ready ? "true" : "false");
}

bool quick_reply_is_ready(void) {
  return s_prompts_ready;
}

void quick_reply_refresh(QuickReply *qr) {
  if (!qr) return;
  update_text_display(qr);
  layer_mark_dirty(qr->layer);
}

