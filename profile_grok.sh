#!/bin/bash
#
# Grebble Grok API Performance Profiler
#
# Tests the xAI Responses API under different configurations to identify
# the bottleneck causing slow responses on the Pebble watch.
#
# Configurations tested:
#   1. Responses API + web_search + x_search (current production config)
#   2. Responses API + web_search only (no x_search)
#   3. Responses API + x_search only (no web_search)
#   4. Responses API + tools with tool_choice=none (tools declared but disabled)
#   5. Responses API + NO tools at all
#   6. Chat Completions API (legacy, no tools)
#
# Each test uses the same prompt and model to isolate the variable.

API_KEY="xai-QwUktPXPPCMQcxr6WOtktn0ijvKnvCymRdCtOSua8ksAUQPtVdzQZ5UF64eIQJkKm4FYfrY4jNWPjpUV"
MODEL="grok-4-1-fast"
SYSTEM_MSG="Respond succinctly in 1-3 sentences max. Do not include sources, citations, or URLs."
RESULTS_FILE="/home/user/grebble/profile_results.md"

# Simple prompt (should NOT need web search)
SIMPLE_PROMPT="Tell me a joke"
# Prompt that benefits from web search
SEARCH_PROMPT="What happened in the news today?"

RUNS_PER_TEST=3

echo "=========================================="
echo "Grebble Grok API Performance Profiler"
echo "=========================================="
echo "Model: $MODEL"
echo "Runs per test: $RUNS_PER_TEST"
echo ""

# Initialize results file
cat > "$RESULTS_FILE" << 'HEADER'
# Grebble Grok API Performance Profile

## Test Environment
- **Model**: grok-4-1-fast
- **Max output tokens**: 256
- **System prompt**: "Respond succinctly in 1-3 sentences max."
- **Runs per test**: 3 (median reported)

## Results

HEADER

run_test() {
    local test_name="$1"
    local url="$2"
    local body="$3"
    local prompt_label="$4"

    echo "--- $test_name ($prompt_label) ---"

    local times=()
    local last_response=""
    local last_status=""

    for i in $(seq 1 $RUNS_PER_TEST); do
        # Use curl with timing
        local start_ms=$(date +%s%3N)

        local response
        local http_code

        # Write response to temp file, capture HTTP code
        response=$(curl -s -w "\n%{http_code}" \
            -X POST "$url" \
            -H "Content-Type: application/json" \
            -H "Authorization: Bearer $API_KEY" \
            --max-time 60 \
            -d "$body" 2>&1)

        local end_ms=$(date +%s%3N)
        local elapsed=$((end_ms - start_ms))

        # Extract HTTP code (last line) and response body
        http_code=$(echo "$response" | tail -1)
        local resp_body=$(echo "$response" | head -n -1)

        last_status="$http_code"
        last_response="$resp_body"

        if [ "$http_code" = "200" ]; then
            times+=($elapsed)
            echo "  Run $i: ${elapsed}ms (HTTP $http_code)"
        else
            echo "  Run $i: ${elapsed}ms (HTTP $http_code - ERROR)"
            times+=($elapsed)
        fi

        # Small delay between runs
        sleep 1
    done

    # Calculate median
    local sorted=($(printf '%s\n' "${times[@]}" | sort -n))
    local mid=$(( ${#sorted[@]} / 2 ))
    local median=${sorted[$mid]}

    # Calculate min and max
    local min=${sorted[0]}
    local max=${sorted[${#sorted[@]}-1]}

    echo "  => Median: ${median}ms  Min: ${min}ms  Max: ${max}ms  Status: $last_status"
    echo ""

    # Extract text from response for verification
    local text_preview=""
    if [ "$http_code" = "200" ]; then
        # Try to extract output text (Responses API format)
        text_preview=$(echo "$last_response" | python3 -c "
import json, sys
try:
    d = json.load(sys.stdin)
    # Responses API
    if 'output' in d:
        for item in d.get('output', []):
            if item.get('type') == 'message':
                for block in item.get('content', []):
                    if block.get('type') in ('output_text', 'text') and block.get('text'):
                        print(block['text'][:100])
                        sys.exit(0)
    # Chat completions
    if 'choices' in d:
        msg = d['choices'][0]['message']['content']
        print(msg[:100])
        sys.exit(0)
    print('[no text extracted]')
except Exception as e:
    print(f'[parse error: {e}]')
" 2>&1)
    fi

    # Check if web_search was actually invoked
    local tools_used=""
    if [ "$http_code" = "200" ]; then
        tools_used=$(echo "$last_response" | python3 -c "
import json, sys
try:
    d = json.load(sys.stdin)
    tools = []
    for item in d.get('output', []):
        t = item.get('type', '')
        if t not in ('message',):
            tools.append(t)
    if tools:
        print(', '.join(tools))
    else:
        print('none')
except:
    print('unknown')
" 2>&1)
    fi

    # Append to results file
    echo "### $test_name ($prompt_label)" >> "$RESULTS_FILE"
    echo "" >> "$RESULTS_FILE"
    echo "| Run | Time (ms) | HTTP Status |" >> "$RESULTS_FILE"
    echo "|-----|-----------|-------------|" >> "$RESULTS_FILE"
    for idx in "${!times[@]}"; do
        echo "| $((idx+1)) | ${times[$idx]} | $last_status |" >> "$RESULTS_FILE"
    done
    echo "" >> "$RESULTS_FILE"
    echo "- **Median**: ${median}ms | **Min**: ${min}ms | **Max**: ${max}ms" >> "$RESULTS_FILE"
    echo "- **Tools invoked by model**: $tools_used" >> "$RESULTS_FILE"
    echo "- **Response preview**: \`${text_preview:0:80}\`" >> "$RESULTS_FILE"
    echo "" >> "$RESULTS_FILE"

    # Return median for summary
    eval "${5}=${median}"
}

echo "=========================================="
echo "TEST SET 1: Simple prompt ('Tell me a joke')"
echo "=========================================="
echo ""

echo "## Simple Prompt: \"Tell me a joke\"" >> "$RESULTS_FILE"
echo "" >> "$RESULTS_FILE"

# Test 1: Current production config - Responses API + both tools
BODY_1=$(cat <<'EOF'
{
  "model": "grok-4-1-fast",
  "max_output_tokens": 256,
  "input": [
    {"role": "system", "content": "Respond succinctly in 1-3 sentences max. Do not include sources, citations, or URLs."},
    {"role": "user", "content": "Tell me a joke"}
  ],
  "tools": [
    {"type": "web_search"},
    {"type": "x_search"}
  ]
}
EOF
)
run_test "1. Responses API + web_search + x_search (CURRENT)" \
    "https://api.x.ai/v1/responses" \
    "$BODY_1" \
    "simple" \
    RESULT_1

# Test 2: Responses API + web_search only
BODY_2=$(cat <<'EOF'
{
  "model": "grok-4-1-fast",
  "max_output_tokens": 256,
  "input": [
    {"role": "system", "content": "Respond succinctly in 1-3 sentences max. Do not include sources, citations, or URLs."},
    {"role": "user", "content": "Tell me a joke"}
  ],
  "tools": [
    {"type": "web_search"}
  ]
}
EOF
)
run_test "2. Responses API + web_search only" \
    "https://api.x.ai/v1/responses" \
    "$BODY_2" \
    "simple" \
    RESULT_2

# Test 3: Responses API + x_search only
BODY_3=$(cat <<'EOF'
{
  "model": "grok-4-1-fast",
  "max_output_tokens": 256,
  "input": [
    {"role": "system", "content": "Respond succinctly in 1-3 sentences max. Do not include sources, citations, or URLs."},
    {"role": "user", "content": "Tell me a joke"}
  ],
  "tools": [
    {"type": "x_search"}
  ]
}
EOF
)
run_test "3. Responses API + x_search only" \
    "https://api.x.ai/v1/responses" \
    "$BODY_3" \
    "simple" \
    RESULT_3

# Test 4: Responses API + tools declared but tool_choice=none
BODY_4=$(cat <<'EOF'
{
  "model": "grok-4-1-fast",
  "max_output_tokens": 256,
  "input": [
    {"role": "system", "content": "Respond succinctly in 1-3 sentences max. Do not include sources, citations, or URLs."},
    {"role": "user", "content": "Tell me a joke"}
  ],
  "tools": [
    {"type": "web_search"},
    {"type": "x_search"}
  ],
  "tool_choice": "none"
}
EOF
)
run_test "4. Responses API + tools + tool_choice=none" \
    "https://api.x.ai/v1/responses" \
    "$BODY_4" \
    "simple" \
    RESULT_4

# Test 5: Responses API with NO tools
BODY_5=$(cat <<'EOF'
{
  "model": "grok-4-1-fast",
  "max_output_tokens": 256,
  "input": [
    {"role": "system", "content": "Respond succinctly in 1-3 sentences max. Do not include sources, citations, or URLs."},
    {"role": "user", "content": "Tell me a joke"}
  ]
}
EOF
)
run_test "5. Responses API + NO tools" \
    "https://api.x.ai/v1/responses" \
    "$BODY_5" \
    "simple" \
    RESULT_5

# Test 6: Chat Completions API (legacy)
BODY_6=$(cat <<'EOF'
{
  "model": "grok-4-1-fast",
  "max_tokens": 256,
  "messages": [
    {"role": "system", "content": "Respond succinctly in 1-3 sentences max. Do not include sources, citations, or URLs."},
    {"role": "user", "content": "Tell me a joke"}
  ]
}
EOF
)
run_test "6. Chat Completions API (legacy, no tools)" \
    "https://api.x.ai/v1/chat/completions" \
    "$BODY_6" \
    "simple" \
    RESULT_6

echo ""
echo "=========================================="
echo "TEST SET 2: Search-heavy prompt ('What happened in the news today?')"
echo "=========================================="
echo ""

echo "## Search-Heavy Prompt: \"What happened in the news today?\"" >> "$RESULTS_FILE"
echo "" >> "$RESULTS_FILE"

# Test 7: Current config with search prompt
BODY_7=$(cat <<'EOF'
{
  "model": "grok-4-1-fast",
  "max_output_tokens": 256,
  "input": [
    {"role": "system", "content": "Respond succinctly in 1-3 sentences max. Do not include sources, citations, or URLs."},
    {"role": "user", "content": "What happened in the news today?"}
  ],
  "tools": [
    {"type": "web_search"},
    {"type": "x_search"}
  ]
}
EOF
)
run_test "7. Responses API + both tools (CURRENT)" \
    "https://api.x.ai/v1/responses" \
    "$BODY_7" \
    "search" \
    RESULT_7

# Test 8: No tools, search prompt
BODY_8=$(cat <<'EOF'
{
  "model": "grok-4-1-fast",
  "max_output_tokens": 256,
  "input": [
    {"role": "system", "content": "Respond succinctly in 1-3 sentences max. Do not include sources, citations, or URLs."},
    {"role": "user", "content": "What happened in the news today?"}
  ]
}
EOF
)
run_test "8. Responses API + NO tools" \
    "https://api.x.ai/v1/responses" \
    "$BODY_8" \
    "search" \
    RESULT_8

# Test 9: web_search only, search prompt
BODY_9=$(cat <<'EOF'
{
  "model": "grok-4-1-fast",
  "max_output_tokens": 256,
  "input": [
    {"role": "system", "content": "Respond succinctly in 1-3 sentences max. Do not include sources, citations, or URLs."},
    {"role": "user", "content": "What happened in the news today?"}
  ],
  "tools": [
    {"type": "web_search"}
  ]
}
EOF
)
run_test "9. Responses API + web_search only" \
    "https://api.x.ai/v1/responses" \
    "$BODY_9" \
    "search" \
    RESULT_9

echo ""
echo "=========================================="
echo "SUMMARY"
echo "=========================================="
echo ""
echo "Simple prompt ('Tell me a joke'):"
echo "  1. Both tools (CURRENT):    ${RESULT_1}ms"
echo "  2. web_search only:         ${RESULT_2}ms"
echo "  3. x_search only:           ${RESULT_3}ms"
echo "  4. tool_choice=none:        ${RESULT_4}ms"
echo "  5. No tools at all:         ${RESULT_5}ms"
echo "  6. Chat Completions (old):  ${RESULT_6}ms"
echo ""
echo "Search prompt ('What happened in the news today?'):"
echo "  7. Both tools (CURRENT):    ${RESULT_7}ms"
echo "  8. No tools:                ${RESULT_8}ms"
echo "  9. web_search only:         ${RESULT_9}ms"
echo ""

# Write summary to results file
cat >> "$RESULTS_FILE" << EOF
## Summary Table

| # | Configuration | Simple (ms) | Search (ms) |
|---|---------------|-------------|-------------|
| 1 | **Responses API + web_search + x_search (CURRENT)** | ${RESULT_1} | ${RESULT_7} |
| 2 | Responses API + web_search only | ${RESULT_2} | ${RESULT_9} |
| 3 | Responses API + x_search only | ${RESULT_3} | — |
| 4 | Responses API + tool_choice=none | ${RESULT_4} | — |
| 5 | Responses API + NO tools | ${RESULT_5} | ${RESULT_8} |
| 6 | Chat Completions API (legacy) | ${RESULT_6} | — |

## Analysis

EOF

echo "Results written to: $RESULTS_FILE"
