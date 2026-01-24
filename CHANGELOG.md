# Changelog

All notable changes to Grebble will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Fixed
- **Pebble Steel/Original Pebble (Aplite) compatibility**: Resolved memory allocation failures that prevented the app from initializing on Aplite devices
  - Reduced AppMessage buffer sizes from 4096 to 512 bytes (with 256 byte fallback)
  - Reduced chat message limits: MAX_MESSAGES 10→6, MESSAGE_TEXT_MAX 512→256, MESSAGE_BUFFER_SIZE 4096→1024
  - Added logging for `app_message_open()` results to aid debugging

### Added
- XHR-based `fetch` polyfill in PebbleKit JS for older runtime compatibility
- Error callbacks on `sendAppMessage()` calls for better failure diagnostics
- Debug instrumentation for tracking message flow between phone and watch
