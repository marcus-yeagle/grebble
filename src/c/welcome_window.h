#pragma once
#include <pebble.h>

/**
 * Welcome Window - Initial screen for Grok for Pebble
 * 
 * Displays the Grok pulse animation with "What do you want to know?" prompt.
 * Space-themed dark aesthetic matching xAI/Grok branding.
 */

Window *welcome_window_create(void);
void welcome_window_destroy(Window *window);

