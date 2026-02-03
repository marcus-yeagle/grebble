#pragma once
#include <pebble.h>

/**
 * RSVP Reader Component
 *
 * Displays text one word (or small chunk) at a time for rapid serial visual presentation.
 * Fullscreen overlay with pause/resume and navigation controls.
 *
 * Controls:
 * - SELECT: pause/resume
 * - UP: jump backward by skip_words
 * - DOWN: jump forward by skip_words
 * - BACK: exit RSVP mode
 */

// RSVP reader configuration (synced from phone settings)
typedef struct {
  bool enabled;             // Feature toggle (default: false)
  uint16_t wpm;             // Words per minute (default: 350, range: 150-800)
  uint8_t words_per_chunk;  // Words to show at once (default: 1, range: 1-3)
  bool pause_on_punctuation;// Pause longer on sentence-ending punctuation (default: true)
  uint8_t skip_words;       // Words to skip on UP/DOWN navigation (default: 5)
  bool show_progress;       // Show progress indicator (default: false)
} RSVPConfig;

typedef struct RSVPReader RSVPReader;

/**
 * Create a new RSVP reader overlay.
 * @param width Width of the overlay (typically screen width)
 * @param height Height of the overlay (available screen height below status bar)
 * @param text The text to display (will be copied internally)
 * @param config RSVP configuration settings
 * @return Pointer to the created RSVP reader, or NULL on failure
 */
RSVPReader* rsvp_reader_create(int width, int height, const char *text, RSVPConfig *config);

/**
 * Destroy an RSVP reader and free its resources.
 * @param reader The reader to destroy
 */
void rsvp_reader_destroy(RSVPReader *reader);

/**
 * Get the underlying Layer for adding to view hierarchy.
 * @param reader The RSVP reader
 * @return The Layer object, or NULL if reader is NULL
 */
Layer* rsvp_reader_get_layer(RSVPReader *reader);

/**
 * Toggle pause/resume state.
 * @param reader The RSVP reader
 */
void rsvp_reader_toggle_pause(RSVPReader *reader);

/**
 * Check if the reader is currently paused.
 * @param reader The RSVP reader
 * @return true if paused, false if playing
 */
bool rsvp_reader_is_paused(RSVPReader *reader);

/**
 * Jump backward by the configured skip amount.
 * @param reader The RSVP reader
 */
void rsvp_reader_skip_back(RSVPReader *reader);

/**
 * Jump forward by the configured skip amount.
 * @param reader The RSVP reader
 */
void rsvp_reader_skip_forward(RSVPReader *reader);

/**
 * Check if playback has finished (reached end of text).
 * @param reader The RSVP reader
 * @return true if finished, false if still playing or paused mid-text
 */
bool rsvp_reader_is_finished(RSVPReader *reader);

/**
 * Get default RSVP configuration.
 * @return Default configuration with standard values
 */
RSVPConfig rsvp_config_get_default(void);
