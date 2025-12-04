#include "chat_footer.h"
#include "grok_pulse.h"

#define PULSE_SIZE 25
#define PADDING 10
#define TEXT_FONT FONT_KEY_GOTHIC_14

struct ChatFooter {
  Layer *layer;
  GrokPulseLayer *pulse;
  TextLayer *text_layer;
  int height;
};

ChatFooter* chat_footer_create(int width) {
  ChatFooter *footer = malloc(sizeof(ChatFooter));
  if (!footer) {
    return NULL;
  }

  // Calculate text dimensions
  int text_x = PADDING + PULSE_SIZE + PADDING;
  int text_width = width - text_x - PADDING;

  GFont font = fonts_get_system_font(TEXT_FONT);
  GSize text_size = graphics_text_layout_get_content_size(
    CHAT_FOOTER_DISCLAIMER_TEXT,
    font,
    GRect(0, 0, text_width, 100),
    GTextOverflowModeWordWrap,
    GTextAlignmentLeft
  );

  // Calculate footer height dynamically (no top padding)
  int content_height = text_size.h > PULSE_SIZE ? text_size.h : PULSE_SIZE;
  footer->height = content_height + PADDING;

  // Create container layer
  footer->layer = layer_create(GRect(0, 0, width, footer->height));

  // Create small Grok pulse on the left (vertically centered in content area)
  int pulse_y = (content_height - PULSE_SIZE) / 2;
  footer->pulse = grok_pulse_layer_create(
    GRect(PADDING, pulse_y, PULSE_SIZE, PULSE_SIZE),
    GROK_PULSE_SMALL
  );
  grok_pulse_set_frame(footer->pulse, 3);  // Static on frame 4
  layer_add_child(footer->layer, grok_pulse_get_layer(footer->pulse));

  // Create disclaimer text on the right (vertically centered in content area)
  int text_y = (content_height - text_size.h) / 2;
  footer->text_layer = text_layer_create(GRect(text_x, text_y, text_width, text_size.h));
  text_layer_set_text(footer->text_layer, CHAT_FOOTER_DISCLAIMER_TEXT);
  text_layer_set_font(footer->text_layer, fonts_get_system_font(TEXT_FONT));
  text_layer_set_text_alignment(footer->text_layer, GTextAlignmentLeft);
  // Dark theme: gray text on transparent background
  text_layer_set_text_color(footer->text_layer, PBL_IF_COLOR_ELSE(GColorDarkGray, GColorLightGray));
  text_layer_set_background_color(footer->text_layer, GColorClear);
  layer_add_child(footer->layer, text_layer_get_layer(footer->text_layer));

  return footer;
}

void chat_footer_destroy(ChatFooter *footer) {
  if (!footer) {
    return;
  }

  if (footer->text_layer) {
    text_layer_destroy(footer->text_layer);
  }

  if (footer->pulse) {
    grok_pulse_layer_destroy(footer->pulse);
  }

  if (footer->layer) {
    layer_destroy(footer->layer);
  }

  free(footer);
}

Layer* chat_footer_get_layer(ChatFooter *footer) {
  return footer ? footer->layer : NULL;
}

void chat_footer_start_animation(ChatFooter *footer) {
  if (footer && footer->pulse) {
    grok_pulse_start_animation(footer->pulse);
  }
}

void chat_footer_stop_animation(ChatFooter *footer) {
  if (footer && footer->pulse) {
    grok_pulse_stop_animation(footer->pulse);
    grok_pulse_set_frame(footer->pulse, 3);  // Back to frame 4
  }
}

int chat_footer_get_height(ChatFooter *footer) {
  return footer ? footer->height : 0;
}

