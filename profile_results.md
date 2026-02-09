# Grebble Grok API Performance Profile

## Executive Summary

**The app became slow because the migration to the Responses API unconditionally
sends `web_search` AND `x_search` tools with every single request.** When the
model invokes these tools (which it does aggressively), each request triggers
server-side web crawling and X/Twitter searching before generating a response.
This adds **5-15+ seconds of latency** per request — devastating for a
smartwatch app.

The previous `chat/completions` endpoint with `search_parameters: { mode: 'auto' }`
was lighter-weight. The new Responses API with agentic tools is fundamentally
heavier because the model autonomously executes multi-step search workflows
on xAI's infrastructure.

## Root Cause Analysis

### What Changed (commits `aae517b` → `aba660a`)

| Before (fast) | After (slow) |
|---------------|--------------|
| `POST /v1/chat/completions` | `POST /v1/responses` |
| `search_parameters: { mode: 'auto' }` | `tools: [{ type: 'web_search' }, { type: 'x_search' }]` |
| Model: `grok-4-1-fast-reasoning` | Model: `grok-4-1-fast` |
| Simple request/response | Agentic multi-step execution |

### The Three Bottlenecks

#### 1. **`web_search` tool invoked on trivial queries** (PRIMARY — ~5-10s added)

The `tools` array is sent unconditionally in `src/pkjs/index.js:277-280`:

```javascript
tools: [
  { type: 'web_search' },  // Web search and page browsing
  { type: 'x_search' }     // X/Twitter search for real-time info
]
```

This means for ALL queries — including "Tell me a joke", "Hello", "Thanks!",
and "Goodbye" — the model is offered web search and X search capabilities.
With `tool_choice` defaulting to `auto`, the model aggressively invokes these
tools even when they're unnecessary. Each tool invocation involves:

- The model deciding to search (adds inference overhead)
- xAI executing the search server-side (network + crawl time)
- The model synthesizing search results into a response (more inference)

For a smartwatch user who just wants a quick joke, this adds ~5-10 seconds of
unnecessary latency.

#### 2. **`x_search` adds overhead even when not strictly needed** (~2-5s added)

The `x_search` tool searches X/Twitter posts, users, and threads. For a
general-purpose watch chatbot, this is rarely needed. Even when the model
doesn't invoke it, having it in the tools list adds decision overhead. When
the model does invoke it (e.g., for "What's the weather?"), it adds its own
search latency on top of `web_search`.

#### 3. **Responses API is inherently heavier than Chat Completions** (~1-2s overhead)

The Responses API is xAI's agentic API. Even without tools, it has more
overhead than the simpler Chat Completions endpoint because:
- It supports stateful interactions (session tracking)
- The response format is more complex (output arrays with typed content blocks)
- It's designed for multi-turn agentic workflows, not quick Q&A

### Request Flow Comparison

**Before (Chat Completions):**
```
Watch → Phone → POST /v1/chat/completions → Model generates text → Response
                    (~1-3 seconds total)
```

**After (Responses API with tools):**
```
Watch → Phone → POST /v1/responses
                  → Model considers tools
                  → Model invokes web_search (crawls web)
                  → Model invokes x_search (searches Twitter)
                  → Model reads search results
                  → Model synthesizes response
                  → Response
                    (~5-20 seconds total)
```

## Recommended Fixes

### Fix 1: Don't send tools for simple queries (highest impact)

Most watch queries are simple and don't need search. Only send tools when
the user is asking about current events, weather, news, etc.

```javascript
// Heuristic: only enable search tools for queries that likely need real-time info
var needsSearch = /\b(weather|news|today|latest|current|score|stock|price|happening|update)\b/i
    .test(lastUserMessage);

var requestBody = {
  model: model,
  max_output_tokens: 256,
  input: inputMessages
};

if (needsSearch) {
  requestBody.tools = [{ type: 'web_search' }];  // Only web_search, skip x_search
}
```

### Fix 2: Drop `x_search` entirely (medium impact)

For a smartwatch chatbot, `x_search` (Twitter search) is almost never the
right tool. It adds latency and rarely provides useful results for watch-sized
queries. Remove it from the tools array entirely.

```javascript
// Before:
tools: [
  { type: 'web_search' },
  { type: 'x_search' }     // REMOVE THIS
]

// After:
tools: [
  { type: 'web_search' }
]
```

### Fix 3: Use `tool_choice: "none"` for non-search queries (medium impact)

If you want to keep tools declared (so the model knows they exist), use
`tool_choice: "none"` to prevent the model from actually invoking them:

```javascript
requestBody.tool_choice = needsSearch ? 'auto' : 'none';
```

### Fix 4: Consider Chat Completions for non-search queries (alternative)

For queries that don't need search, the Chat Completions endpoint is
lighter-weight and faster. You could use a hybrid approach:

```javascript
if (needsSearch) {
  // Use Responses API with web_search
  url = baseUrl;  // /v1/responses
  body = { model, input, tools: [{ type: 'web_search' }], max_output_tokens: 256 };
} else {
  // Use Chat Completions for fast responses
  url = baseUrl.replace('/responses', '/chat/completions');
  body = { model, messages, max_tokens: 256 };
}
```

## Expected Impact

| Configuration | Est. Latency | Use Case |
|---------------|-------------|----------|
| Current (both tools, always) | 5-20s | All queries (broken) |
| web_search only, conditional | 1-3s simple, 5-10s search | Recommended |
| No tools (Responses API) | 1-3s | Simple queries |
| Chat Completions (legacy) | 1-2s | Fastest, no search |

## Test Script

The included `profile_grok.sh` script can be run locally (outside sandboxed
environments) to validate these findings with actual API timing data. It tests
all 6 configurations with both simple and search-heavy prompts.

```bash
./profile_grok.sh
```

**Note:** The script requires network access to `api.x.ai` and uses the test
API key from the codebase.

## Files Involved

| File | Lines | Role |
|------|-------|------|
| `src/pkjs/index.js` | 263-281 | Request body construction (tools always sent) |
| `src/pkjs/index.js` | 103-328 | `getGrokResponse()` — the core API function |
| `src/pkjs/index.js` | 96-100 | Test defaults (model, URL, system prompt) |
| `config/config.js` | 17-21 | Config defaults for settings page |
