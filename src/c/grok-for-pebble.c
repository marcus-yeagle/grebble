/**
 * Grok for Pebble - Main Entry Point
 * 
 * A Pebble smartwatch client for chatting with Grok AI from xAI.
 * Features canned prompts and conversation history.
 * 
 * Adapted from Claude for Pebble, unaffiliated with xAI.
 */

#include <pebble.h>
#include "grok_pulse.h"
#include "chat_window.h"
#include "setup_window.h"
#include "quick_reply.h"

static Window *s_chat_window;
static Window *s_setup_window;
static bool s_is_ready = true;  // Assume ready initially, will be corrected by JS

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Check for READY_STATUS message
  Tuple *ready_status_tuple = dict_find(iterator, MESSAGE_KEY_READY_STATUS);
  if (ready_status_tuple) {
    int status = ready_status_tuple->value->int32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Received READY_STATUS: %d", status);

    bool new_ready_state = (status == 1);

    // If status changed, update windows
    if (s_is_ready != new_ready_state) {
      s_is_ready = new_ready_state;

      if (new_ready_state) {
        // App is now configured - pop the setup window to reveal chat underneath
        APP_LOG(APP_LOG_LEVEL_DEBUG, "App configured, removing setup window");

        if (s_setup_window && window_stack_contains_window(s_setup_window)) {
          window_stack_remove(s_setup_window, true);
        }
      } else {
        // Need to show setup window
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Transitioning to setup window");

        // Create setup window if needed
        if (!s_setup_window) {
          s_setup_window = setup_window_create();
        }

        // Push setup window
        if (!window_stack_contains_window(s_setup_window)) {
          window_stack_push(s_setup_window, true);
        }
      }
    }

    // Don't return - also check for canned prompts in same message
  }

  // Check for canned prompts from phone settings
  Tuple *prompt_tuple;
  
  prompt_tuple = dict_find(iterator, MESSAGE_KEY_CANNED_PROMPT_1);
  if (prompt_tuple && prompt_tuple->value->cstring) {
    quick_reply_set_prompt(0, prompt_tuple->value->cstring);
  }
  
  prompt_tuple = dict_find(iterator, MESSAGE_KEY_CANNED_PROMPT_2);
  if (prompt_tuple && prompt_tuple->value->cstring) {
    quick_reply_set_prompt(1, prompt_tuple->value->cstring);
  }
  
  prompt_tuple = dict_find(iterator, MESSAGE_KEY_CANNED_PROMPT_3);
  if (prompt_tuple && prompt_tuple->value->cstring) {
    quick_reply_set_prompt(2, prompt_tuple->value->cstring);
  }
  
  prompt_tuple = dict_find(iterator, MESSAGE_KEY_CANNED_PROMPT_4);
  if (prompt_tuple && prompt_tuple->value->cstring) {
    quick_reply_set_prompt(3, prompt_tuple->value->cstring);
  }
  
  prompt_tuple = dict_find(iterator, MESSAGE_KEY_CANNED_PROMPT_5);
  if (prompt_tuple && prompt_tuple->value->cstring) {
    quick_reply_set_prompt(4, prompt_tuple->value->cstring);
  }

  // Forward other messages to chat window handler
  chat_window_handle_inbox(iterator);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped: %d", (int)reason);
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed: %d", (int)reason);
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Outbox send success!");
}

static void prv_init(void) {
  // Initialize Grok pulse system
  grok_pulse_init();
  
  // Initialize quick reply system (for custom canned prompts)
  quick_reply_init();

  // Initialize AppMessage
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage with large inbox (for history) and large outbox (for chunks)
  app_message_open(4096, 4096);

  // Go directly to chat window with quick reply selector
  s_chat_window = chat_window_create();
  window_stack_push(s_chat_window, true);
}

static void prv_deinit(void) {
  // Destroy windows
  if (s_chat_window) {
    chat_window_destroy(s_chat_window);
    s_chat_window = NULL;
  }

  if (s_setup_window) {
    setup_window_destroy(s_setup_window);
    s_setup_window = NULL;
  }

  // Deinitialize Grok pulse system
  grok_pulse_deinit();
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
