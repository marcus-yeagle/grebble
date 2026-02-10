#!/bin/bash
#
# Grebble Multi-Model Performance Profiler
#
# Tests multiple Grok models head-to-head under identical conditions
# to identify the fastest model for smartwatch use.
#
# Test matrix per model:
#   A. Responses API, no tools (pure speed baseline)
#   B. Responses API + web_search (production config)
#   C. Chat Completions API, no tools (legacy baseline)
#
# Prompts:
#   1. Simple: "Tell me a joke" (no search needed)
#   2. Search: "What happened in the news today?" (benefits from web_search)
#
# Usage:
#   ./profile_models.sh                    # Run with embedded test key
#   XAI_API_KEY=xai-xxx ./profile_models.sh  # Run with custom key
#   RUNS=5 ./profile_models.sh             # 5 runs per test (default 3)

set -euo pipefail

# ─── Configuration ───────────────────────────────────────────────
API_KEY="${XAI_API_KEY:-xai-QwUktPXPPCMQcxr6WOtktn0ijvKnvCymRdCtOSua8ksAUQPtVdzQZ5UF64eIQJkKm4FYfrY4jNWPjpUV}"
SYSTEM_MSG="Respond succinctly in 1-3 sentences max. Do not include sources, citations, or URLs."
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
RESULTS_FILE="${SCRIPT_DIR}/model_profile_results.md"
RUNS_PER_TEST="${RUNS:-3}"

# Models to test (add/remove as needed)
MODELS=(
  "grok-4-1-fast"
  "grok-4-fast"
  "grok-4"
)

SIMPLE_PROMPT="Tell me a joke"
SEARCH_PROMPT="What happened in the news today?"

# Temp file to store results (key=value pairs) — avoids bash 4+ associative arrays
RESULTS_STORE=$(mktemp /tmp/grebble_profile.XXXXXX)
trap "rm -f $RESULTS_STORE" EXIT

store_result() {
  echo "$1=$2" >> "$RESULTS_STORE"
}

get_result() {
  local val
  val=$(grep "^$1=" "$RESULTS_STORE" 2>/dev/null | tail -1 | cut -d= -f2-)
  echo "${val:-$2}"
}

# ─── Helpers ─────────────────────────────────────────────────────

# Cross-platform millisecond timestamp (macOS date doesn't support %3N)
now_ms() {
  python3 -c "import time; print(int(time.time() * 1000))"
}

# Build JSON request body
build_body() {
  local model="$1"
  local config_name="$2"
  local prompt="$3"

  case "$config_name" in
    responses_no_tools)
      python3 -c "
import json, sys
print(json.dumps({
  'model': sys.argv[1],
  'max_output_tokens': 256,
  'input': [
    {'role': 'system', 'content': sys.argv[2]},
    {'role': 'user', 'content': sys.argv[3]}
  ]
}))" "$model" "$SYSTEM_MSG" "$prompt"
      ;;
    responses_web_search)
      python3 -c "
import json, sys
print(json.dumps({
  'model': sys.argv[1],
  'max_output_tokens': 256,
  'input': [
    {'role': 'system', 'content': sys.argv[2]},
    {'role': 'user', 'content': sys.argv[3]}
  ],
  'tools': [
    {'type': 'web_search'}
  ]
}))" "$model" "$SYSTEM_MSG" "$prompt"
      ;;
    chat_completions)
      python3 -c "
import json, sys
print(json.dumps({
  'model': sys.argv[1],
  'max_tokens': 256,
  'messages': [
    {'role': 'system', 'content': sys.argv[2]},
    {'role': 'user', 'content': sys.argv[3]}
  ]
}))" "$model" "$SYSTEM_MSG" "$prompt"
      ;;
  esac
}

# Extract response text from API response JSON
extract_text() {
  python3 -c "
import json, sys
try:
    d = json.load(sys.stdin)
    if 'output' in d:
        for item in d.get('output', []):
            if item.get('type') == 'message':
                for block in item.get('content', []):
                    if block.get('type') in ('output_text', 'text') and block.get('text'):
                        print(block['text'][:100])
                        sys.exit(0)
    if 'choices' in d:
        msg = d['choices'][0]['message']['content']
        print(msg[:100])
        sys.exit(0)
    print('[no text extracted]')
except Exception as e:
    print(f'[parse error: {e}]')
" 2>&1
}

# Extract tools used from Responses API output
extract_tools() {
  python3 -c "
import json, sys
try:
    d = json.load(sys.stdin)
    tools = []
    for item in d.get('output', []):
        t = item.get('type', '')
        if t not in ('message',):
            tools.append(t)
    print(', '.join(tools) if tools else 'none')
except:
    print('unknown')
" 2>&1
}

# ─── Test runner ─────────────────────────────────────────────────

TEST_NUM=0
TOTAL_TESTS=$(( ${#MODELS[@]} * 3 * 2 ))  # models * configs * prompts

run_test() {
  local label="$1"
  local url="$2"
  local body="$3"
  local result_key="$4"

  TEST_NUM=$((TEST_NUM + 1))
  echo "  [$TEST_NUM/$TOTAL_TESTS] $label"

  local times=()
  local last_response=""
  local last_status=""
  local error_count=0

  for i in $(seq 1 "$RUNS_PER_TEST"); do
    local start_ms
    start_ms=$(now_ms)

    local response
    response=$(curl -s -w "\n%{http_code}" \
      -X POST "$url" \
      -H "Content-Type: application/json" \
      -H "Authorization: Bearer $API_KEY" \
      --max-time 60 \
      -d "$body" 2>&1)

    local end_ms
    end_ms=$(now_ms)
    local elapsed=$((end_ms - start_ms))

    local http_code
    http_code=$(echo "$response" | tail -1)
    local resp_body
    resp_body=$(echo "$response" | sed '$d')

    last_status="$http_code"
    last_response="$resp_body"

    if [ "$http_code" = "200" ]; then
      times+=("$elapsed")
      echo "           Run $i: ${elapsed}ms (HTTP $http_code)"
    else
      error_count=$((error_count + 1))
      times+=("$elapsed")
      echo "           Run $i: ${elapsed}ms (HTTP $http_code - ERROR)"
      if [ "$i" -eq 1 ] && [ "$http_code" != "200" ]; then
        echo "           Skipping remaining runs (model/config unavailable)"
        break
      fi
    fi

    [ "$i" -lt "$RUNS_PER_TEST" ] && sleep 1
  done

  # Calculate median, min, max
  local median=0 min=0 max=0
  if [ ${#times[@]} -gt 0 ]; then
    local sorted
    sorted=($(printf '%s\n' "${times[@]}" | sort -n))
    local mid=$(( ${#sorted[@]} / 2 ))
    median=${sorted[$mid]}
    min=${sorted[0]}
    max=${sorted[${#sorted[@]}-1]}
  fi

  # Extract text preview and tools used
  local text_preview=""
  local tools_used="n/a"
  if [ "$last_status" = "200" ]; then
    text_preview=$(echo "$last_response" | extract_text)
    tools_used=$(echo "$last_response" | extract_tools)
  fi

  # Store results in temp file
  store_result "median_${result_key}" "$median"
  store_result "errors_${result_key}" "$error_count"

  # Write detailed results to file
  {
    echo "### $label"
    echo ""
    echo "| Run | Time (ms) | HTTP Status |"
    echo "|-----|-----------|-------------|"
    for idx in "${!times[@]}"; do
      echo "| $((idx+1)) | ${times[$idx]} | $last_status |"
    done
    echo ""
    echo "- **Median**: ${median}ms | **Min**: ${min}ms | **Max**: ${max}ms"
    echo "- **Tools invoked**: $tools_used"
    echo "- **Response**: \`${text_preview:0:80}\`"
    echo ""
  } >> "$RESULTS_FILE"

  echo "           => Median: ${median}ms  (errors: $error_count)"
}

# ─── Main ────────────────────────────────────────────────────────

echo "=========================================="
echo "Grebble Multi-Model Performance Profiler"
echo "=========================================="
echo "Models: ${MODELS[*]}"
echo "Configs: 3 per model (no tools, web_search, chat completions)"
echo "Prompts: simple + search"
echo "Runs per test: $RUNS_PER_TEST"
echo "Total API calls: $((TOTAL_TESTS * RUNS_PER_TEST))"
echo "=========================================="
echo ""

# Initialize results file
cat > "$RESULTS_FILE" << EOF
# Grebble Multi-Model Performance Profile

**Date**: $(date '+%Y-%m-%d %H:%M:%S')
**Models tested**: ${MODELS[*]}
**Runs per test**: $RUNS_PER_TEST (median reported)
**System prompt**: "$SYSTEM_MSG"

## Detailed Results

EOF

CONFIG_NAMES=("responses_no_tools" "responses_web_search" "chat_completions")
CONFIG_URLS=("https://api.x.ai/v1/responses" "https://api.x.ai/v1/responses" "https://api.x.ai/v1/chat/completions")

for model in "${MODELS[@]}"; do
  echo ""
  echo "=== Model: $model ==="
  {
    echo ""
    echo "---"
    echo "## Model: \`$model\`"
    echo ""
  } >> "$RESULTS_FILE"

  for cfg_idx in 0 1 2; do
    config_name="${CONFIG_NAMES[$cfg_idx]}"
    url="${CONFIG_URLS[$cfg_idx]}"

    for prompt_label in "simple" "search"; do
      case "$prompt_label" in
        simple) prompt="$SIMPLE_PROMPT" ;;
        search) prompt="$SEARCH_PROMPT" ;;
      esac

      body=$(build_body "$model" "$config_name" "$prompt")
      label="$model | $config_name | $prompt_label"
      result_key="${model}__${config_name}__${prompt_label}"

      run_test "$label" "$url" "$body" "$result_key"
    done
  done
done

# ─── Summary tables ──────────────────────────────────────────────

echo ""
echo "=========================================="
echo "RESULTS SUMMARY"
echo "=========================================="
echo ""

{
  echo "---"
  echo ""
  echo "## Full Results Matrix"
  echo ""
  echo "| Model | Config | Simple (ms) | Search (ms) | Errors |"
  echo "|-------|--------|-------------|-------------|--------|"
} >> "$RESULTS_FILE"

for model in "${MODELS[@]}"; do
  for cfg_idx in 0 1 2; do
    config_name="${CONFIG_NAMES[$cfg_idx]}"

    key_simple="${model}__${config_name}__simple"
    key_search="${model}__${config_name}__search"

    ms_simple=$(get_result "median_${key_simple}" "—")
    ms_search=$(get_result "median_${key_search}" "—")
    err_simple=$(get_result "errors_${key_simple}" "0")
    err_search=$(get_result "errors_${key_search}" "0")
    total_err=$((err_simple + err_search))

    case "$config_name" in
      responses_no_tools)   pretty="No tools" ;;
      responses_web_search) pretty="+ web_search" ;;
      chat_completions)     pretty="Chat Completions" ;;
      *)                    pretty="$config_name" ;;
    esac

    echo "| \`$model\` | $pretty | $ms_simple | $ms_search | $total_err |" >> "$RESULTS_FILE"

    printf "  %-20s | %-16s | simple: %5s ms | search: %5s ms | errors: %d\n" \
      "$model" "$pretty" "$ms_simple" "$ms_search" "$total_err"
  done
done

# ─── Speed ranking ───────────────────────────────────────────────

echo ""
echo "--- Speed Ranking (Responses API, no tools, simple prompt) ---"
echo ""

{
  echo ""
  echo "## Speed Ranking"
  echo ""
  echo "Ranked by **Responses API, no tools, simple prompt** (most common smartwatch interaction):"
  echo ""
  echo "| Rank | Model | Median (ms) | vs Current (\`grok-4-1-fast\`) |"
  echo "|------|-------|-------------|-------------------------------|"
} >> "$RESULTS_FILE"

# Collect and sort ranking data using python for simplicity
RANKING=$(python3 -c "
import sys

models = sys.argv[1:]
data = []
for m in models:
    key = f'{m}__responses_no_tools__simple'
    median_line = [l for l in open('$RESULTS_STORE') if l.startswith(f'median_{key}=')]
    error_line = [l for l in open('$RESULTS_STORE') if l.startswith(f'errors_{key}=')]
    median = int(median_line[-1].split('=')[1].strip()) if median_line else 999999
    errors = int(error_line[-1].split('=')[1].strip()) if error_line else 99
    data.append((m, median, errors))

# Sort by median time
data.sort(key=lambda x: x[1])

# Find baseline
baseline = next((d[1] for d in data if d[0] == 'grok-4-1-fast'), 1)

for rank, (model, ms, errs) in enumerate(data, 1):
    if model == 'grok-4-1-fast':
        pct = 'baseline'
    elif baseline > 0:
        diff = ((ms - baseline) / baseline) * 100
        pct = f'{diff:+.0f}%'
    else:
        pct = 'n/a'
    print(f'{rank}|{model}|{ms}|{pct}|{errs}')
" "${MODELS[@]}")

WINNER_MODEL=""
WINNER_MS=""

echo "$RANKING" | while IFS='|' read -r rank model ms pct errs; do
  echo "  #$rank  $model  ${ms}ms  ($pct)"
done

# Write ranking to results file and find winner
while IFS='|' read -r rank model ms pct errs; do
  echo "| $rank | \`$model\` | $ms | $pct |" >> "$RESULTS_FILE"
  if [ -z "$WINNER_MODEL" ]; then
    WINNER_MODEL="$model"
    WINNER_MS="$ms"
  fi
done <<< "$RANKING"

# Get winner's search time
winner_search_ms=$(get_result "median_${WINNER_MODEL}__responses_web_search__search" "unknown")

# ─── Recommendation ──────────────────────────────────────────────

{
  echo ""
  echo "## Recommendation"
  echo ""
  echo "**Fastest model for smartwatch use: \`$WINNER_MODEL\`** (${WINNER_MS}ms median for simple queries)"
  echo ""
  echo "- Simple queries (no tools): ~${WINNER_MS}ms"
  echo "- Search queries (web_search): ~${winner_search_ms}ms"
  echo ""
  if [ "$WINNER_MODEL" != "grok-4-1-fast" ]; then
    echo "### To apply this change, update:"
    echo ""
    echo "1. \`src/pkjs/index.js\` line 99: \`TEST_MODEL = '$WINNER_MODEL'\`"
    echo "2. \`src/pkjs/index.js\` line 534: \`defaultModel = '$WINNER_MODEL'\`"
    echo "3. \`src/pkjs/index.js\` line 707: List \`$WINNER_MODEL\` first with \"(fastest)\""
    echo "4. \`config/config.js\` line 19: \`model: '$WINNER_MODEL'\`"
  else
    echo "Current default \`grok-4-1-fast\` is already the fastest. No changes needed."
  fi
} >> "$RESULTS_FILE"

echo ""
echo "=========================================="
echo "WINNER: $WINNER_MODEL (${WINNER_MS}ms)"
echo "=========================================="
echo ""
echo "Results written to: $RESULTS_FILE"
