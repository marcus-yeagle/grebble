#include "g_loader.h"

/*
 * 3x3 dot grid with a brightness snake that traces a G-shaped path
 * through the 9 fixed grid positions. Matches the Grok-style loader.
 *
 * Grid indices (row-major):
 *   0  1  2
 *   3  4  5
 *   6  7  8
 *
 * G-path visit order:
 *   2 -> 1 -> 0 -> 3 -> 6 -> 7 -> 8 -> 5 -> 4
 *   (top-right, top-center, top-left, mid-left, bot-left,
 *    bot-center, bot-right, mid-right, center)
 */

#define GRID_SIZE 3
#define DOT_COUNT (GRID_SIZE * GRID_SIZE)
#define PATH_LEN 9
#define TRAIL_LEN 3
#define NUM_FRAMES PATH_LEN
#define ANIM_INTERVAL_MS 120
#define PAUSE_INTERVAL_MS 400
#define DOT_RADIUS 2
#define DOT_SPACING 8

static const int G_ORDER[PATH_LEN] = {
  2, 1, 0, 3, 6, 7, 8, 5, 4
};

struct GLoaderLayer {
  Layer *layer;
  AppTimer *timer;
  int frame_index;
  bool is_animating;
};

static void update_proc(Layer *layer, GContext *ctx);
static void next_frame_handler(void *context);

static GPoint grid_point(GRect bounds, int idx) {
  int col = idx % GRID_SIZE;
  int row = idx / GRID_SIZE;
  int total = (GRID_SIZE - 1) * DOT_SPACING;
  int ox = (bounds.size.w - total) / 2;
  int oy = (bounds.size.h - total) / 2;
  return GPoint(ox + col * DOT_SPACING, oy + row * DOT_SPACING);
}

GLoaderLayer* g_loader_layer_create(GRect frame) {
  GLoaderLayer *loader = malloc(sizeof(GLoaderLayer));
  if (!loader) return NULL;

  loader->layer = layer_create_with_data(frame, sizeof(GLoaderLayer*));
  loader->timer = NULL;
  loader->frame_index = 0;
  loader->is_animating = false;

  layer_set_update_proc(loader->layer, update_proc);
  *((GLoaderLayer**)layer_get_data(loader->layer)) = loader;

  return loader;
}

void g_loader_layer_destroy(GLoaderLayer *loader) {
  if (!loader) return;
  if (loader->timer) app_timer_cancel(loader->timer);
  if (loader->layer) layer_destroy(loader->layer);
  free(loader);
}

Layer* g_loader_get_layer(GLoaderLayer *loader) {
  return loader ? loader->layer : NULL;
}

void g_loader_start_animation(GLoaderLayer *loader) {
  if (!loader || loader->is_animating) return;
  loader->is_animating = true;
  loader->frame_index = 0;
  layer_mark_dirty(loader->layer);
  loader->timer = app_timer_register(ANIM_INTERVAL_MS, next_frame_handler, loader);
}

void g_loader_stop_animation(GLoaderLayer *loader) {
  if (!loader) return;
  loader->is_animating = false;
  if (loader->timer) {
    app_timer_cancel(loader->timer);
    loader->timer = NULL;
  }
}

void g_loader_set_frame(GLoaderLayer *loader, int frame_index) {
  if (!loader) return;
  g_loader_stop_animation(loader);
  loader->frame_index = frame_index % NUM_FRAMES;
  layer_mark_dirty(loader->layer);
}

bool g_loader_is_animating(GLoaderLayer *loader) {
  return loader ? loader->is_animating : false;
}

static void update_proc(Layer *layer, GContext *ctx) {
  GLoaderLayer *loader = *((GLoaderLayer**)layer_get_data(layer));
  if (!loader) return;

  GRect bounds = layer_get_bounds(layer);
  int head = loader->frame_index;

  /* Determine brightness level for each of the 9 grid dots.
   * 0 = inactive, 1..TRAIL_LEN = in trail (1 = brightest / head) */
  int brightness[DOT_COUNT];
  for (int i = 0; i < DOT_COUNT; i++) brightness[i] = 0;

  for (int t = 0; t < TRAIL_LEN; t++) {
    int path_idx = (head - t + PATH_LEN) % PATH_LEN;
    int grid_idx = G_ORDER[path_idx];
    if (brightness[grid_idx] == 0) {
      brightness[grid_idx] = t + 1;
    }
  }

  for (int i = 0; i < DOT_COUNT; i++) {
    GPoint pt = grid_point(bounds, i);
    int b = brightness[i];

#ifdef PBL_COLOR
    GColor color;
    if (b == 1)      color = GColorWhite;
    else if (b == 2) color = GColorLightGray;
    else if (b == 3) color = GColorDarkGray;
    else             color = GColorBlack;

    if (b == 0) {
      graphics_context_set_fill_color(ctx, GColorDarkGray);
      graphics_fill_circle(ctx, pt, DOT_RADIUS);
    } else {
      graphics_context_set_fill_color(ctx, color);
      graphics_fill_circle(ctx, pt, DOT_RADIUS);
    }
#else
    if (b == 1) {
      graphics_context_set_fill_color(ctx, GColorWhite);
      graphics_fill_circle(ctx, pt, DOT_RADIUS);
    } else if (b > 1) {
      graphics_context_set_fill_color(ctx, GColorWhite);
      graphics_fill_circle(ctx, pt, 1);
    } else {
      graphics_context_set_stroke_color(ctx, GColorWhite);
      graphics_context_set_stroke_width(ctx, 1);
      graphics_draw_circle(ctx, pt, DOT_RADIUS);
    }
#endif
  }
}

static void next_frame_handler(void *context) {
  GLoaderLayer *loader = (GLoaderLayer*)context;
  if (!loader || !loader->is_animating) return;

  loader->frame_index = (loader->frame_index + 1) % NUM_FRAMES;
  layer_mark_dirty(loader->layer);

  int delay = (loader->frame_index == 0) ? PAUSE_INTERVAL_MS : ANIM_INTERVAL_MS;
  loader->timer = app_timer_register(delay, next_frame_handler, loader);
}
