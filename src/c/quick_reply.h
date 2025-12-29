#pragma once
#include <pebble.h>

/**
 * Quick Reply Component
 *
 * Overlay UI for selecting pre-defined quick reply messages.
 * Shows current selection with UP/DOWN navigation hints.
 * Used as iOS dictation workaround.
 * 
 * Supports custom prompts configured via the phone settings app.
 */

#define NUM_CANNED_PROMPTS 5
#define MAX_PROMPT_LENGTH 64

typedef struct QuickReply QuickReply;

/**
 * Create a new quick reply overlay.
 * @param width Width of the overlay (typically screen width)
 * @return Pointer to the created quick reply component
 */
QuickReply* quick_reply_create(int width);

/**
 * Create a fullscreen quick reply overlay.
 * @param width Width of the overlay (typically screen width)
 * @param height Height of the overlay (typically available screen height)
 * @return Pointer to the created quick reply component
 */
QuickReply* quick_reply_create_fullscreen(int width, int height);

/**
 * Destroy a quick reply overlay and free its resources.
 * @param qr The quick reply to destroy
 */
void quick_reply_destroy(QuickReply *qr);

/**
 * Get the underlying Layer for adding to view hierarchy.
 * @param qr The quick reply component
 * @return The Layer object
 */
Layer* quick_reply_get_layer(QuickReply *qr);

/**
 * Get the height of the quick reply overlay.
 * @param qr The quick reply component
 * @return Height in pixels
 */
int quick_reply_get_height(QuickReply *qr);

/**
 * Move to the next quick reply option.
 * @param qr The quick reply component
 */
void quick_reply_next(QuickReply *qr);

/**
 * Move to the previous quick reply option.
 * @param qr The quick reply component
 */
void quick_reply_prev(QuickReply *qr);

/**
 * Get the currently selected quick reply text.
 * @param qr The quick reply component
 * @return The selected message text
 */
const char* quick_reply_get_selected(QuickReply *qr);

/**
 * Get the current selection index.
 * @param qr The quick reply component
 * @return The current index
 */
int quick_reply_get_index(QuickReply *qr);

/**
 * Get the total number of quick replies.
 * @param qr The quick reply component
 * @return Total count
 */
int quick_reply_get_count(QuickReply *qr);

/**
 * Set a custom canned prompt at the specified index.
 * Call this to configure prompts from phone settings.
 * @param index Index (0-4) of the prompt to set
 * @param prompt The prompt text (max 63 chars, will be truncated)
 */
void quick_reply_set_prompt(int index, const char *prompt);

/**
 * Initialize the quick reply system with default prompts.
 * Should be called at app startup.
 */
void quick_reply_init(void);

/**
 * Get a canned prompt at the specified index.
 * @param index Index (0-4) of the prompt to get
 * @return The prompt text, or NULL if invalid index
 */
const char* quick_reply_get_prompt(int index);

/**
 * Set whether prompts are ready (received from phone).
 * While not ready, shows "Loading..." instead of prompts.
 * @param ready true when phone has synced, false during loading
 */
void quick_reply_set_ready(bool ready);

/**
 * Check if prompts are ready to display.
 * @return true if ready, false if still loading
 */
bool quick_reply_is_ready(void);

/**
 * Refresh the display of a quick reply component.
 * Call this after prompts have been loaded to update "Loading..." to actual prompts.
 * @param qr The quick reply component to refresh (can be NULL)
 */
void quick_reply_refresh(QuickReply *qr);

