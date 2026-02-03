#include "rsvp_reader.h"
#include <string.h>
#include <stdlib.h>

#define RSVP_PADDING 10
#define RSVP_WORD_FONT FONT_KEY_GOTHIC_28_BOLD
#define RSVP_PROGRESS_FONT FONT_KEY_GOTHIC_14
#define RSVP_STATUS_FONT FONT_KEY_GOTHIC_18

// Maximum words we can handle (memory constraint)
#ifdef PBL_PLATFORM_APLITE
  #define MAX_WORDS 128
#else
  #define MAX_WORDS 256
#endif

struct RSVPReader {
  Layer *layer;
  TextLayer *word_layer;
  TextLayer *progress_layer;
  TextLayer *status_layer;  // Shows "PAUSED" indicator
  
  char *text_buffer;        // Owned copy of the text
  char **words;             // Array of pointers into text_buffer
  int word_count;           // Total number of words
  int current_word;         // Current word index
  
  RSVPConfig config;
  AppTimer *timer;
  bool paused;
  bool finished;
  
  int width;
  int height;
  
  // Buffer for displaying current chunk
  char display_buffer[128];
};

// Forward declarations
static void rsvp_timer_callback(void *context);
static void schedule_next_word(RSVPReader *reader);
static void update_display(RSVPReader *reader);
static void tokenize_text(RSVPReader *reader);
static int calculate_delay(RSVPReader *reader);
static bool ends_with_major_punctuation(const char *word);
static bool ends_with_minor_punctuation(const char *word);

static void rsvp_update_proc(Layer *layer, GContext *ctx) {
  RSVPReader *reader = *(RSVPReader **)layer_get_data(layer);
  if (!reader) return;
  
  GRect bounds = layer_get_bounds(layer);
  
  // Fill with dark background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
}

RSVPConfig rsvp_config_get_default(void) {
  RSVPConfig config;
  config.enabled = false;
  config.wpm = 350;
  config.words_per_chunk = 1;
  config.pause_on_punctuation = true;
  config.skip_words = 5;
  config.show_progress = false;
  return config;
}

RSVPReader* rsvp_reader_create(int width, int height, const char *text, RSVPConfig *config) {
  if (!text || strlen(text) == 0) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "RSVP: No text provided");
    return NULL;
  }
  
  RSVPReader *reader = malloc(sizeof(RSVPReader));
  if (!reader) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "RSVP: Failed to allocate reader");
    return NULL;
  }
  
  memset(reader, 0, sizeof(RSVPReader));
  reader->width = width;
  reader->height = height;
  reader->paused = false;
  reader->finished = false;
  reader->current_word = 0;
  
  // Copy configuration
  if (config) {
    reader->config = *config;
  } else {
    reader->config = rsvp_config_get_default();
  }
  
  // Copy text (we need to modify it for tokenization)
  size_t text_len = strlen(text);
  reader->text_buffer = malloc(text_len + 1);
  if (!reader->text_buffer) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "RSVP: Failed to allocate text buffer");
    free(reader);
    return NULL;
  }
  strcpy(reader->text_buffer, text);
  
  // Allocate word pointer array
  reader->words = malloc(sizeof(char*) * MAX_WORDS);
  if (!reader->words) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "RSVP: Failed to allocate word array");
    free(reader->text_buffer);
    free(reader);
    return NULL;
  }
  
  // Tokenize the text into words
  tokenize_text(reader);
  
  if (reader->word_count == 0) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "RSVP: No words found in text");
    free(reader->words);
    free(reader->text_buffer);
    free(reader);
    return NULL;
  }
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "RSVP: Tokenized %d words", reader->word_count);
  
  // Create container layer
  reader->layer = layer_create_with_data(GRect(0, 0, width, height), sizeof(RSVPReader*));
  RSVPReader **layer_data = (RSVPReader **)layer_get_data(reader->layer);
  *layer_data = reader;
  layer_set_update_proc(reader->layer, rsvp_update_proc);
  
  // Calculate vertical center for word display
  int word_height = 36;  // Approximate height for GOTHIC_28_BOLD
  int word_y = (height - word_height) / 2 - 10;
  
  // Create word text layer (centered)
  reader->word_layer = text_layer_create(GRect(RSVP_PADDING, word_y, width - 2 * RSVP_PADDING, word_height + 20));
  text_layer_set_font(reader->word_layer, fonts_get_system_font(RSVP_WORD_FONT));
  text_layer_set_text_alignment(reader->word_layer, GTextAlignmentCenter);
  text_layer_set_text_color(reader->word_layer, GColorWhite);
  text_layer_set_background_color(reader->word_layer, GColorClear);
  text_layer_set_overflow_mode(reader->word_layer, GTextOverflowModeWordWrap);
  layer_add_child(reader->layer, text_layer_get_layer(reader->word_layer));
  
  // Create progress layer (bottom, optional)
  if (reader->config.show_progress) {
    int progress_y = height - 24;
    reader->progress_layer = text_layer_create(GRect(RSVP_PADDING, progress_y, width - 2 * RSVP_PADDING, 20));
    text_layer_set_font(reader->progress_layer, fonts_get_system_font(RSVP_PROGRESS_FONT));
    text_layer_set_text_alignment(reader->progress_layer, GTextAlignmentCenter);
    text_layer_set_text_color(reader->progress_layer, PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite));
    text_layer_set_background_color(reader->progress_layer, GColorClear);
    layer_add_child(reader->layer, text_layer_get_layer(reader->progress_layer));
  }
  
  // Create status layer (top, for pause indicator)
  reader->status_layer = text_layer_create(GRect(RSVP_PADDING, 8, width - 2 * RSVP_PADDING, 24));
  text_layer_set_font(reader->status_layer, fonts_get_system_font(RSVP_STATUS_FONT));
  text_layer_set_text_alignment(reader->status_layer, GTextAlignmentCenter);
  text_layer_set_text_color(reader->status_layer, PBL_IF_COLOR_ELSE(GColorVividCerulean, GColorWhite));
  text_layer_set_background_color(reader->status_layer, GColorClear);
  layer_add_child(reader->layer, text_layer_get_layer(reader->status_layer));
  
  // Show first word and start playback
  update_display(reader);
  schedule_next_word(reader);
  
  return reader;
}

void rsvp_reader_destroy(RSVPReader *reader) {
  if (!reader) return;
  
  // Cancel any pending timer
  if (reader->timer) {
    app_timer_cancel(reader->timer);
    reader->timer = NULL;
  }
  
  // Destroy text layers
  if (reader->status_layer) {
    text_layer_destroy(reader->status_layer);
  }
  if (reader->progress_layer) {
    text_layer_destroy(reader->progress_layer);
  }
  if (reader->word_layer) {
    text_layer_destroy(reader->word_layer);
  }
  
  // Destroy container layer
  if (reader->layer) {
    layer_destroy(reader->layer);
  }
  
  // Free allocated memory
  if (reader->words) {
    free(reader->words);
  }
  if (reader->text_buffer) {
    free(reader->text_buffer);
  }
  
  free(reader);
}

Layer* rsvp_reader_get_layer(RSVPReader *reader) {
  return reader ? reader->layer : NULL;
}

void rsvp_reader_toggle_pause(RSVPReader *reader) {
  if (!reader) return;
  
  reader->paused = !reader->paused;
  
  if (reader->paused) {
    // Cancel timer when pausing
    if (reader->timer) {
      app_timer_cancel(reader->timer);
      reader->timer = NULL;
    }
    text_layer_set_text(reader->status_layer, "PAUSED");
  } else {
    // Resume playback
    text_layer_set_text(reader->status_layer, "");
    if (!reader->finished) {
      schedule_next_word(reader);
    }
  }
  
  layer_mark_dirty(reader->layer);
}

bool rsvp_reader_is_paused(RSVPReader *reader) {
  return reader ? reader->paused : false;
}

void rsvp_reader_skip_back(RSVPReader *reader) {
  if (!reader) return;
  
  int skip = reader->config.skip_words;
  reader->current_word -= skip;
  if (reader->current_word < 0) {
    reader->current_word = 0;
  }
  
  reader->finished = false;
  update_display(reader);
  
  // If not paused, reschedule timer
  if (!reader->paused) {
    if (reader->timer) {
      app_timer_cancel(reader->timer);
      reader->timer = NULL;
    }
    schedule_next_word(reader);
  }
}

void rsvp_reader_skip_forward(RSVPReader *reader) {
  if (!reader) return;
  
  int skip = reader->config.skip_words;
  reader->current_word += skip;
  if (reader->current_word >= reader->word_count) {
    reader->current_word = reader->word_count - 1;
    reader->finished = true;
  }
  
  update_display(reader);
  
  // If not paused and not finished, reschedule timer
  if (!reader->paused && !reader->finished) {
    if (reader->timer) {
      app_timer_cancel(reader->timer);
      reader->timer = NULL;
    }
    schedule_next_word(reader);
  }
}

bool rsvp_reader_is_finished(RSVPReader *reader) {
  return reader ? reader->finished : true;
}

// --- Private functions ---

// Check if character is a whitespace delimiter
static bool is_whitespace(char c) {
  return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

static void tokenize_text(RSVPReader *reader) {
  reader->word_count = 0;
  
  // Manual tokenization to avoid strtok global state issues
  char *buf = reader->text_buffer;
  char *word_start = NULL;
  
  while (*buf && reader->word_count < MAX_WORDS) {
    if (is_whitespace(*buf)) {
      // End of word - null-terminate it
      if (word_start) {
        *buf = '\0';
        reader->words[reader->word_count++] = word_start;
        word_start = NULL;
      }
    } else {
      // Start of a new word
      if (!word_start) {
        word_start = buf;
      }
    }
    buf++;
  }
  
  // Don't forget the last word if text doesn't end with whitespace
  if (word_start && reader->word_count < MAX_WORDS) {
    reader->words[reader->word_count++] = word_start;
  }
}

static void update_display(RSVPReader *reader) {
  if (!reader || reader->word_count == 0) return;
  
  // Build display text from current word(s)
  reader->display_buffer[0] = '\0';
  
  int words_to_show = reader->config.words_per_chunk;
  int end_word = reader->current_word + words_to_show;
  if (end_word > reader->word_count) {
    end_word = reader->word_count;
  }
  
  for (int i = reader->current_word; i < end_word; i++) {
    if (i > reader->current_word) {
      strncat(reader->display_buffer, " ", sizeof(reader->display_buffer) - strlen(reader->display_buffer) - 1);
    }
    strncat(reader->display_buffer, reader->words[i], sizeof(reader->display_buffer) - strlen(reader->display_buffer) - 1);
  }
  
  text_layer_set_text(reader->word_layer, reader->display_buffer);
  
  // Update progress if enabled
  if (reader->progress_layer) {
    static char progress_text[32];
    snprintf(progress_text, sizeof(progress_text), "%d / %d", reader->current_word + 1, reader->word_count);
    text_layer_set_text(reader->progress_layer, progress_text);
  }
  
  layer_mark_dirty(reader->layer);
}

static bool ends_with_major_punctuation(const char *word) {
  if (!word || strlen(word) == 0) return false;
  char last = word[strlen(word) - 1];
  return (last == '.' || last == '!' || last == '?' || last == ':');
}

static bool ends_with_minor_punctuation(const char *word) {
  if (!word || strlen(word) == 0) return false;
  char last = word[strlen(word) - 1];
  return (last == ',' || last == ';');
}

static int calculate_delay(RSVPReader *reader) {
  // Base delay: ms_per_word = 60000 / wpm
  int base_delay = 60000 / reader->config.wpm;
  
  // Apply punctuation multiplier if enabled
  if (reader->config.pause_on_punctuation && reader->current_word < reader->word_count) {
    // Get the last word in the current chunk
    int last_word_idx = reader->current_word + reader->config.words_per_chunk - 1;
    if (last_word_idx >= reader->word_count) {
      last_word_idx = reader->word_count - 1;
    }
    
    const char *word = reader->words[last_word_idx];
    
    if (ends_with_major_punctuation(word)) {
      // 2x delay for major punctuation (. ! ? :)
      base_delay *= 2;
    } else if (ends_with_minor_punctuation(word)) {
      // 1.5x delay for minor punctuation (, ;)
      base_delay = (base_delay * 3) / 2;
    }
  }
  
  return base_delay;
}

static void schedule_next_word(RSVPReader *reader) {
  if (!reader || reader->paused || reader->finished) return;
  
  int delay = calculate_delay(reader);
  reader->timer = app_timer_register(delay, rsvp_timer_callback, reader);
}

static void rsvp_timer_callback(void *context) {
  RSVPReader *reader = (RSVPReader *)context;
  if (!reader) return;
  
  reader->timer = NULL;
  
  if (reader->paused) return;
  
  // Advance by words_per_chunk
  reader->current_word += reader->config.words_per_chunk;
  
  if (reader->current_word >= reader->word_count) {
    // Reached the end
    reader->current_word = reader->word_count - 1;
    reader->finished = true;
    update_display(reader);
    text_layer_set_text(reader->status_layer, "END");
    return;
  }
  
  update_display(reader);
  schedule_next_word(reader);
}
