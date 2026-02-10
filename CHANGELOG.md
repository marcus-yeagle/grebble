# Changelog

All notable changes to Grebble will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added
- **"xAI" attribution label**: Grok response bubbles now show a subtle "xAI" label at the bottom-right
- **RSVP Reader**: New rapid serial visual presentation mode for reading Grok responses word-by-word on the watch
  - Configurable WPM (150–800), words per chunk (1–3), and punctuation pausing
  - Controls: SELECT to pause/resume, UP/DOWN to skip backward/forward, BACK to exit
  - Progress indicator option and settings synced from phone
- Responses API parsing for `output` message blocks (`output_text` / `text`)
- Error callbacks on `sendAppMessage()` calls for better failure diagnostics

### Changed
- **Loading animation**: Replaced the pulsing "X" logo with a 3×3 dot matrix grid that animates in a snake-spiral wave pattern; idle state now shows an empty frame (no dots)
- **Default model**: Changed from `grok-4-1-fast` to `grok-4-fast`; model migration updated accordingly
- **Animation timing**: Increased frame interval from 80 ms to 120 ms (14 frames, up from 12) for a smoother, less frenetic pulse
- **xAI defaults**: Updated default endpoint to `https://api.x.ai/v1/responses` and default model to `grok-4-fast`
- **Web search integration**: Switched from deprecated `search_parameters` to Responses API Agent Tools (`web_search`)
- **Removed `x_search` tool**: X/Twitter search added 2–5s of latency per request and was rarely useful for a smartwatch chatbot; only `web_search` is now sent

### Fixed
- **Pebble Steel/Original Pebble (Aplite) compatibility**: Resolved memory allocation failures that prevented the app from initializing on Aplite devices
  - Reduced AppMessage buffer sizes from 4096 to 512 bytes (with 256 byte fallback)
  - Reduced chat message limits: MAX_MESSAGES 10→6, MESSAGE_TEXT_MAX 512→256, MESSAGE_BUFFER_SIZE 4096→1024
  - Added logging for `app_message_open()` results to aid debugging
- **Web search on phone builds**: Fixed cases where a previously-saved legacy `base_url` (like `/v1/chat/completions` or `/v1/messages`) disabled tool use on real devices
  - Automatically migrates legacy xAI endpoints to `https://api.x.ai/v1/responses`
- **Emulator settings redirect**: Settings pages now respect `return_to` so Save/Reset/Send work correctly in the emulator
