/**
 * Grebble - Main Entry Point
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

    // Mark quick reply as ready - phone has synced, show prompts instead of "Loading..."
    quick_reply_set_ready(true);

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

  // Refresh quick reply display if it's showing (updates from "Loading..." to prompts)
  chat_window_refresh_quick_reply();

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

  // Open AppMessage.
  //
  // Aplite (original Pebble/Pebble Steel) has a much smaller memory budget than
  // later platforms; using very large AppMessage buffers can fail, which then
  // causes the phone-side READY_STATUS send to NACK.
  AppMessageResult open_result = APP_MSG_OK;
  uint32_t inbox_size = 2048;
  uint32_t outbox_size = 2048;

#ifdef PBL_PLATFORM_APLITE
  inbox_size = 512;
  outbox_size = 512;
#endif

  open_result = app_message_open(inbox_size, outbox_size);
  APP_LOG(open_result == APP_MSG_OK ? APP_LOG_LEVEL_DEBUG : APP_LOG_LEVEL_ERROR,
          "app_message_open(in=%lu,out=%lu) => %d",
          (unsigned long)inbox_size, (unsigned long)outbox_size, (int)open_result);

#ifdef PBL_PLATFORM_APLITE
  // If this still fails on aplite, retry with a smaller size to keep the app usable.
  if (open_result != APP_MSG_OK) {
    inbox_size = 256;
    outbox_size = 256;
    open_result = app_message_open(inbox_size, outbox_size);
    APP_LOG(open_result == APP_MSG_OK ? APP_LOG_LEVEL_DEBUG : APP_LOG_LEVEL_ERROR,
            "app_message_open(in=%lu,out=%lu) => %d",
            (unsigned long)inbox_size, (unsigned long)outbox_size, (int)open_result);
  }
#endif

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

