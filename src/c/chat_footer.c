#include "chat_footer.h"
#include "g_loader.h"

#define LOADER_SIZE 25
#define PADDING 8

struct ChatFooter {
  Layer *layer;
  GLoaderLayer *loader;
  int height;
};

ChatFooter* chat_footer_create(int width) {
  ChatFooter *footer = malloc(sizeof(ChatFooter));
  if (!footer) {
    return NULL;
  }

  footer->height = LOADER_SIZE + PADDING;
  footer->layer = layer_create(GRect(0, 0, width, footer->height));

  int lx = (width - LOADER_SIZE) / 2;
  int ly = PADDING / 2;
  footer->loader = g_loader_layer_create(GRect(lx, ly, LOADER_SIZE, LOADER_SIZE));
  g_loader_set_frame(footer->loader, 3);
  layer_add_child(footer->layer, g_loader_get_layer(footer->loader));

  return footer;
}

void chat_footer_destroy(ChatFooter *footer) {
  if (!footer) {
    return;
  }

  if (footer->loader) {
    g_loader_layer_destroy(footer->loader);
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
  if (footer && footer->loader) {
    g_loader_start_animation(footer->loader);
  }
}

void chat_footer_stop_animation(ChatFooter *footer) {
  if (footer && footer->loader) {
    g_loader_stop_animation(footer->loader);
    g_loader_set_frame(footer->loader, 3);
  }
}

int chat_footer_get_height(ChatFooter *footer) {
  return footer ? footer->height : 0;
}
