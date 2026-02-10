#include "message_bubble.h"

#define MESSAGE_PADDING 8
#define MESSAGE_FONT FONT_KEY_GOTHIC_18_BOLD
#define XAI_LABEL_HEIGHT 16

struct MessageBubble {
  Layer *layer;
  TextLayer *text_layer;
  TextLayer *xai_label;
  bool is_user;
  int max_width;
};

static void background_update_proc(Layer *layer, GContext *ctx) {
  MessageBubble *bubble = *(MessageBubble**)layer_get_data(layer);
  if (!bubble) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);

  // Dark theme styling
  if (bubble->is_user) {
    // User messages: dark blue/gray highlight on color, light gray on B&W
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorDarkGray, GColorLightGray));
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  }
}

MessageBubble* message_bubble_create(const char *text, bool is_user, int max_width) {
  MessageBubble *bubble = malloc(sizeof(MessageBubble));
  if (!bubble) {
    return NULL;
  }

  bubble->is_user = is_user;
  bubble->max_width = max_width;

  // Calculate text size (account for padding so bubble doesn't exceed max_width)
  GFont font = fonts_get_system_font(MESSAGE_FONT);
  int available_text_width = max_width - (MESSAGE_PADDING * 2);
  GSize text_size = graphics_text_layout_get_content_size(
    text,
    font,
    GRect(0, 0, available_text_width, 2000),
    GTextOverflowModeWordWrap,
    GTextAlignmentLeft
  );

  // Bubble spans full width, height based on text + padding
  // Grok messages get extra height for the "xAI" label
  int label_extra = is_user ? 0 : XAI_LABEL_HEIGHT;
  int bubble_height = text_size.h + (MESSAGE_PADDING * 2) + label_extra;

  // Create container layer with background (full width)
  bubble->layer = layer_create_with_data(GRect(0, 0, max_width, bubble_height), sizeof(MessageBubble*));
  layer_set_update_proc(bubble->layer, background_update_proc);
  *(MessageBubble**)layer_get_data(bubble->layer) = bubble;

  // Create text layer (positioned to center vertically, with extra height for descenders)
  bubble->text_layer = text_layer_create(GRect(
    MESSAGE_PADDING,
    MESSAGE_PADDING / 2,
    text_size.w,
    bubble_height - MESSAGE_PADDING
  ));
  text_layer_set_text(bubble->text_layer, text);
  text_layer_set_font(bubble->text_layer, font);
  text_layer_set_text_alignment(bubble->text_layer, GTextAlignmentLeft);
  text_layer_set_background_color(bubble->text_layer, GColorClear);
  
  // Dark theme text colors
  // User: white text on dark background
  // Grok: light gray/white text on black background
  text_layer_set_text_color(bubble->text_layer, is_user ? GColorWhite : PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite));
  
  layer_add_child(bubble->layer, text_layer_get_layer(bubble->text_layer));

  // Add "xAI" label at bottom-right of Grok messages
  bubble->xai_label = NULL;
  if (!is_user) {
    int label_y = text_size.h + MESSAGE_PADDING;
    bubble->xai_label = text_layer_create(GRect(0, label_y, max_width - MESSAGE_PADDING, XAI_LABEL_HEIGHT));
    text_layer_set_text(bubble->xai_label, "xAI");
    text_layer_set_font(bubble->xai_label, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(bubble->xai_label, GTextAlignmentRight);
    text_layer_set_background_color(bubble->xai_label, GColorClear);
    text_layer_set_text_color(bubble->xai_label, PBL_IF_COLOR_ELSE(GColorDarkGray, GColorLightGray));
    layer_add_child(bubble->layer, text_layer_get_layer(bubble->xai_label));
  }

  return bubble;
}

void message_bubble_destroy(MessageBubble *bubble) {
  if (!bubble) {
    return;
  }

  if (bubble->xai_label) {
    text_layer_destroy(bubble->xai_label);
  }

  if (bubble->text_layer) {
    text_layer_destroy(bubble->text_layer);
  }

  if (bubble->layer) {
    layer_destroy(bubble->layer);
  }

  free(bubble);
}

Layer* message_bubble_get_layer(MessageBubble *bubble) {
  return bubble ? bubble->layer : NULL;
}

int message_bubble_get_height(MessageBubble *bubble) {
  if (!bubble || !bubble->layer) {
    return 0;
  }

  return layer_get_frame(bubble->layer).size.h;
}

