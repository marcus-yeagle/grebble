#pragma once
#include <pebble.h>

/**
 * Grok Pulse Animation Component
 *
 * Provides reusable Grok pulse animation (pulsing star/eye effect) 
 * that can be placed anywhere in the UI. Space-themed aesthetic
 * matching xAI/Grok branding.
 * 
 * Supports two sizes (large/small) and animation control (play/pause/freeze).
 */

typedef enum {
  GROK_PULSE_SMALL,
  GROK_PULSE_LARGE
} GrokPulseSize;

typedef struct GrokPulseLayer GrokPulseLayer;

/**
 * Initialize the Grok Pulse system.
 * Call this once during app initialization.
 */
void grok_pulse_init(void);

/**
 * Deinitialize the Grok Pulse system.
 * Call this once during app deinitialization.
 */
void grok_pulse_deinit(void);

/**
 * Create a new Grok Pulse layer.
 * @param frame The position and size of the layer
 * @param size The size variant (SMALL or LARGE)
 * @return Pointer to the created pulse layer
 */
GrokPulseLayer* grok_pulse_layer_create(GRect frame, GrokPulseSize size);

/**
 * Destroy a Grok Pulse layer and free its resources.
 * @param pulse The pulse layer to destroy
 */
void grok_pulse_layer_destroy(GrokPulseLayer *pulse);

/**
 * Get the underlying Layer for adding to the view hierarchy.
 * @param pulse The pulse layer
 * @return The Layer object
 */
Layer* grok_pulse_get_layer(GrokPulseLayer *pulse);

/**
 * Start animating the pulse (loops through animation).
 * @param pulse The pulse layer
 */
void grok_pulse_start_animation(GrokPulseLayer *pulse);

/**
 * Stop the animation and freeze on the current frame.
 * @param pulse The pulse layer
 */
void grok_pulse_stop_animation(GrokPulseLayer *pulse);

/**
 * Set the pulse to display a specific frame (and stop animation).
 * @param pulse The pulse layer
 * @param frame_index The frame index (0-based)
 */
void grok_pulse_set_frame(GrokPulseLayer *pulse, int frame_index);

/**
 * Change the size of the pulse.
 * @param pulse The pulse layer
 * @param size The new size
 */
void grok_pulse_set_size(GrokPulseLayer *pulse, GrokPulseSize size);

/**
 * Check if the pulse is currently animating.
 * @param pulse The pulse layer
 * @return true if animating, false otherwise
 */
bool grok_pulse_is_animating(GrokPulseLayer *pulse);

