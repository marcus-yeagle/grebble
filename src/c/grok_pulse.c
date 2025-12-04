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

#define NUM_FRAMES 12
#define ANIMATION_INTERVAL_MS 80

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

// Draw a stylized "X" with pulsing effect - represents xAI's logo
static void draw_grok_x(GContext *ctx, GPoint center, int size, int frame) {
  // Calculate pulse factor (0.0 to 1.0 based on frame)
  // Using a smooth sine-like pulse
  int pulse_offset = (frame < NUM_FRAMES/2) ? frame : (NUM_FRAMES - frame);
  int pulse_factor = pulse_offset * 2;  // 0 to NUM_FRAMES range
  
  // Base dimensions
  int arm_length = size / 2;
  int inner_gap = size / 6 + (pulse_factor * size) / (NUM_FRAMES * 8);
  
  // Calculate stroke width based on size
  int stroke_width = (size > 40) ? 3 : 2;
  
  // Color - electric blue on color displays, black on B&W
  GColor draw_color = PBL_IF_COLOR_ELSE(GColorVividCerulean, GColorBlack);
  graphics_context_set_stroke_color(ctx, draw_color);
  graphics_context_set_stroke_width(ctx, stroke_width);
  
  // Draw the X shape with gap in center for "eye" effect
  // Top-left to center-ish
  GPoint tl_outer = GPoint(center.x - arm_length, center.y - arm_length);
  GPoint tl_inner = GPoint(center.x - inner_gap, center.y - inner_gap);
  graphics_draw_line(ctx, tl_outer, tl_inner);
  
  // Top-right to center-ish
  GPoint tr_outer = GPoint(center.x + arm_length, center.y - arm_length);
  GPoint tr_inner = GPoint(center.x + inner_gap, center.y - inner_gap);
  graphics_draw_line(ctx, tr_outer, tr_inner);
  
  // Bottom-left to center-ish
  GPoint bl_outer = GPoint(center.x - arm_length, center.y + arm_length);
  GPoint bl_inner = GPoint(center.x - inner_gap, center.y + inner_gap);
  graphics_draw_line(ctx, bl_outer, bl_inner);
  
  // Bottom-right to center-ish
  GPoint br_outer = GPoint(center.x + arm_length, center.y + arm_length);
  GPoint br_inner = GPoint(center.x + inner_gap, center.y + inner_gap);
  graphics_draw_line(ctx, br_outer, br_inner);
  
  // Draw pulsing center dot (the "eye")
  int dot_radius = 2 + (pulse_factor * 2) / NUM_FRAMES;
  graphics_context_set_fill_color(ctx, draw_color);
  graphics_fill_circle(ctx, center, dot_radius);
  
  // Draw outer glow ring on color displays during animation
#ifdef PBL_COLOR
  if (pulse_factor > NUM_FRAMES / 4) {
    int ring_radius = inner_gap + 2;
    graphics_context_set_stroke_width(ctx, 1);
    graphics_context_set_stroke_color(ctx, GColorPictonBlue);
    graphics_draw_circle(ctx, center, ring_radius);
  }
#endif
}

// Draw radiating star points around the X
static void draw_star_points(GContext *ctx, GPoint center, int size, int frame) {
  // Only draw star points during certain frames for twinkling effect
  int twinkle_phase = frame % 4;
  if (twinkle_phase == 0) return;
  
  GColor star_color = PBL_IF_COLOR_ELSE(GColorPictonBlue, GColorBlack);
  graphics_context_set_stroke_color(ctx, star_color);
  graphics_context_set_stroke_width(ctx, 1);
  
  int ray_length = size / 4;
  int ray_offset = size / 2 + 4;
  
  // Draw cardinal direction rays (N, E, S, W)
  // Only some rays based on twinkle phase
  if (twinkle_phase >= 1) {
    // North ray
    graphics_draw_line(ctx, 
      GPoint(center.x, center.y - ray_offset),
      GPoint(center.x, center.y - ray_offset - ray_length));
  }
  if (twinkle_phase >= 2) {
    // East ray
    graphics_draw_line(ctx,
      GPoint(center.x + ray_offset, center.y),
      GPoint(center.x + ray_offset + ray_length, center.y));
    // West ray
    graphics_draw_line(ctx,
      GPoint(center.x - ray_offset, center.y),
      GPoint(center.x - ray_offset - ray_length, center.y));
  }
  if (twinkle_phase >= 3) {
    // South ray
    graphics_draw_line(ctx,
      GPoint(center.x, center.y + ray_offset),
      GPoint(center.x, center.y + ray_offset + ray_length));
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
  
  // Draw the Grok X symbol with pulse effect
  draw_grok_x(ctx, center, draw_size, pulse->frame_index);
  
  // Draw twinkling star points for large size
  if (pulse->size == GROK_PULSE_LARGE) {
    draw_star_points(ctx, center, draw_size, pulse->frame_index);
  }
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

