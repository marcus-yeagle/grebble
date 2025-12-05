#include "chat_footer.h"
#include "grok_pulse.h"

#define PULSE_SIZE 25
#define PADDING 8

struct ChatFooter {
  Layer *layer;
  GrokPulseLayer *pulse;
  int height;
};

ChatFooter* chat_footer_create(int width) {
  ChatFooter *footer = malloc(sizeof(ChatFooter));
  if (!footer) {
    return NULL;
  }

  // Footer height is just enough for the centered pulse icon
  footer->height = PULSE_SIZE + PADDING;

  // Create container layer
  footer->layer = layer_create(GRect(0, 0, width, footer->height));

  // Create small Grok pulse centered horizontally
  int pulse_x = (width - PULSE_SIZE) / 2;
  int pulse_y = PADDING / 2;
  footer->pulse = grok_pulse_layer_create(
    GRect(pulse_x, pulse_y, PULSE_SIZE, PULSE_SIZE),
    GROK_PULSE_SMALL
  );
  grok_pulse_set_frame(footer->pulse, 3);  // Static on frame 4
  layer_add_child(footer->layer, grok_pulse_get_layer(footer->pulse));

  return footer;
}

void chat_footer_destroy(ChatFooter *footer) {
  if (!footer) {
    return;
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

