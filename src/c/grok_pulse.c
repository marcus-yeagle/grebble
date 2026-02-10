#include "grok_pulse.h"

/**
 * Grok Pulse Animation
 * 
 * A pulsing star/eye animation representing Grok's "truth-seeking" nature.
 * Programmatically drawn to avoid external resource dependencies.
 * Features:
 * - Pulsing center point (eye/star core)
 * - Radiating lines that grow/shrink
 * - Electric blue on color displays, black on B&W
 */

#define NUM_FRAMES 14
#define ANIMATION_INTERVAL_MS 120
#define DOT_GRID_SIZE 3

// Individual pulse layer instance
struct GrokPulseLayer {
  Layer *layer;
  AppTimer *timer;
  int frame_index;
  bool is_animating;
  GrokPulseSize size;
};

// Forward declarations
static void update_proc(Layer *layer, GContext *ctx);
static void next_frame_handler(void *context);

void grok_pulse_init(void) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Grok pulse system initialized");
}

void grok_pulse_deinit(void) {
  // Nothing to clean up since we draw programmatically
}

GrokPulseLayer* grok_pulse_layer_create(GRect frame, GrokPulseSize size) {
  GrokPulseLayer *pulse = malloc(sizeof(GrokPulseLayer));
  if (!pulse) {
    return NULL;
  }

  pulse->layer = layer_create_with_data(frame, sizeof(GrokPulseLayer*));
  pulse->timer = NULL;
  pulse->frame_index = 0;
  pulse->is_animating = false;
  pulse->size = size;

  layer_set_update_proc(pulse->layer, update_proc);
  *((GrokPulseLayer**)layer_get_data(pulse->layer)) = pulse;

  return pulse;
}

void grok_pulse_layer_destroy(GrokPulseLayer *pulse) {
  if (!pulse) {
    return;
  }

  if (pulse->timer) {
    app_timer_cancel(pulse->timer);
  }

  if (pulse->layer) {
    layer_destroy(pulse->layer);
  }

  free(pulse);
}

Layer* grok_pulse_get_layer(GrokPulseLayer *pulse) {
  return pulse ? pulse->layer : NULL;
}

void grok_pulse_start_animation(GrokPulseLayer *pulse) {
  if (!pulse || pulse->is_animating) {
    return;
  }

  pulse->is_animating = true;
  pulse->frame_index = 0;

  layer_mark_dirty(pulse->layer);
  pulse->timer = app_timer_register(ANIMATION_INTERVAL_MS, next_frame_handler, pulse);
}

void grok_pulse_stop_animation(GrokPulseLayer *pulse) {
  if (!pulse) {
    return;
  }

  pulse->is_animating = false;

  if (pulse->timer) {
    app_timer_cancel(pulse->timer);
    pulse->timer = NULL;
  }
}

void grok_pulse_set_frame(GrokPulseLayer *pulse, int frame_index) {
  if (!pulse) {
    return;
  }

  grok_pulse_stop_animation(pulse);
  pulse->frame_index = frame_index % NUM_FRAMES;
  layer_mark_dirty(pulse->layer);
}

void grok_pulse_set_size(GrokPulseLayer *pulse, GrokPulseSize size) {
  if (!pulse) {
    return;
  }

  bool was_animating = pulse->is_animating;
  grok_pulse_stop_animation(pulse);

  pulse->size = size;
  layer_mark_dirty(pulse->layer);

  if (was_animating) {
    grok_pulse_start_animation(pulse);
  }
}

bool grok_pulse_is_animating(GrokPulseLayer *pulse) {
  return pulse ? pulse->is_animating : false;
}

// Draw a 3x3 dot matrix grid with scattered wave animation
static void draw_dot_matrix(GContext *ctx, GPoint center, int size, int frame) {
  // Snake spiral from top-right, counter-clockwise, inward to center
  // Path: [0,2] [0,1] [0,0] [1,0] [2,0] [2,1] [2,2] [1,2] [1,1]
  //          0     1     2     3     4     5     6     7     8
  static const int phase_map[DOT_GRID_SIZE][DOT_GRID_SIZE] = {
    {2, 1, 0},
    {3, 8, 7},
    {4, 5, 6}
  };

  // Dot sizing based on overall size
  int base_radius = (size > 40) ? 4 : 2;
  int active_radius = base_radius + ((size > 40) ? 2 : 1);
  int spacing = size / (DOT_GRID_SIZE + 1);

  // Grid offset to center the 3x3 grid
  int grid_offset = spacing * (DOT_GRID_SIZE - 1) / 2;

  // Colors
  GColor active_color = PBL_IF_COLOR_ELSE(GColorVividCerulean, GColorWhite);
  GColor dim_color = PBL_IF_COLOR_ELSE(GColorDarkGray, GColorLightGray);

  for (int row = 0; row < DOT_GRID_SIZE; row++) {
    for (int col = 0; col < DOT_GRID_SIZE; col++) {
      int dot_phase = phase_map[row][col];
      int active_level = (frame - dot_phase + NUM_FRAMES) % NUM_FRAMES;

      GPoint dot_center = GPoint(
        center.x - grid_offset + col * spacing,
        center.y - grid_offset + row * spacing
      );

      if (active_level == 0) {
        // Peak: bright and large
        graphics_context_set_fill_color(ctx, active_color);
        graphics_fill_circle(ctx, dot_center, active_radius);
      } else if (active_level == 1) {
        // Fading: medium brightness, base size
        GColor fade_color = PBL_IF_COLOR_ELSE(GColorPictonBlue, GColorWhite);
        graphics_context_set_fill_color(ctx, fade_color);
        graphics_fill_circle(ctx, dot_center, base_radius);
      } else if (active_level == 2) {
        // Fading out: dim, base size
        graphics_context_set_fill_color(ctx, dim_color);
        graphics_fill_circle(ctx, dot_center, base_radius);
      }
      // active_level >= 3: dot not drawn (empty)
    }
  }
}

static void update_proc(Layer *layer, GContext *ctx) {
  GrokPulseLayer *pulse = *((GrokPulseLayer**)layer_get_data(layer));
  if (!pulse) {
    return;
  }

  GRect bounds = layer_get_bounds(layer);
  GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2);
  
  // Determine size based on pulse size setting
  int draw_size = (pulse->size == GROK_PULSE_SMALL) ? 20 : 50;
  
  // Draw dot matrix grid with wave animation
  draw_dot_matrix(ctx, center, draw_size, pulse->frame_index);
}

static void next_frame_handler(void *context) {
  GrokPulseLayer *pulse = (GrokPulseLayer*)context;
  if (!pulse || !pulse->is_animating) {
    return;
  }

  // Advance to next frame
  pulse->frame_index++;
  if (pulse->frame_index >= NUM_FRAMES) {
    pulse->frame_index = 0;
  }

  // Redraw
  layer_mark_dirty(pulse->layer);

  // Schedule next frame
  pulse->timer = app_timer_register(ANIMATION_INTERVAL_MS, next_frame_handler, pulse);
}

