# Grebble Multi-Model Performance Profile

**Date**: 2026-02-10 09:33:42
**Models tested**: grok-4-1-fast grok-4-fast grok-4
**Runs per test**: 3 (median reported)
**System prompt**: "Respond succinctly in 1-3 sentences max. Do not include sources, citations, or URLs."

## Detailed Results


---
## Model: `grok-4-1-fast`

### grok-4-1-fast | responses_no_tools | simple

| Run | Time (ms) | HTTP Status |
|-----|-----------|-------------|
| 1 | 1613 | 200 |
| 2 | 1705 | 200 |
| 3 | 1438 | 200 |

- **Median**: 1613ms | **Min**: 1438ms | **Max**: 1705ms
- **Tools invoked**: none
- **Response**: `Why don't scientists trust atoms? Because they make up everything!`

### grok-4-1-fast | responses_no_tools | search

| Run | Time (ms) | HTTP Status |
|-----|-----------|-------------|
| 1 | 5560 | 200 |
| 2 | 5387 | 200 |
| 3 | 2680 | 200 |

- **Median**: 5387ms | **Min**: 2680ms | **Max**: 5560ms
- **Tools invoked**: none
- **Response**: `I don't have real-time access to news, so I can't provide today's updates. Check`

### grok-4-1-fast | responses_web_search | simple

| Run | Time (ms) | HTTP Status |
|-----|-----------|-------------|
| 1 | 2011 | 200 |
| 2 | 1883 | 200 |
| 3 | 1681 | 200 |

- **Median**: 1883ms | **Min**: 1681ms | **Max**: 2011ms
- **Tools invoked**: none
- **Response**: `Why don't scientists trust atoms? Because they make up everything!`

### grok-4-1-fast | responses_web_search | search

| Run | Time (ms) | HTTP Status |
|-----|-----------|-------------|
| 1 | 25065 | 200 |
| 2 | 16133 | 200 |
| 3 | 20453 | 200 |

- **Median**: 20453ms | **Min**: 16133ms | **Max**: 25065ms
- **Tools invoked**: web_search_call, web_search_call, web_search_call
- **Response**: `**Top U.S. headlines include President Trump threatening to block a new Canadian`

### grok-4-1-fast | chat_completions | simple

| Run | Time (ms) | HTTP Status |
|-----|-----------|-------------|
| 1 | 1580 | 200 |
| 2 | 1222 | 200 |
| 3 | 1489 | 200 |

- **Median**: 1489ms | **Min**: 1222ms | **Max**: 1580ms
- **Tools invoked**: none
- **Response**: `Why don't scientists trust atoms? Because they make up everything!`

### grok-4-1-fast | chat_completions | search

| Run | Time (ms) | HTTP Status |
|-----|-----------|-------------|
| 1 | 3576 | 200 |
| 2 | 5374 | 200 |
| 3 | 5989 | 200 |

- **Median**: 5374ms | **Min**: 3576ms | **Max**: 5989ms
- **Tools invoked**: none
- **Response**: `Major headlines today include Donald Trump's projected victory in the US preside`


---
## Model: `grok-4-fast`

### grok-4-fast | responses_no_tools | simple

| Run | Time (ms) | HTTP Status |
|-----|-----------|-------------|
| 1 | 867 | 200 |
| 2 | 1035 | 200 |
| 3 | 900 | 200 |

- **Median**: 900ms | **Min**: 867ms | **Max**: 1035ms
- **Tools invoked**: none
- **Response**: `Why did the scarecrow win an award? Because he was outstanding in his field!`

### grok-4-fast | responses_no_tools | search

| Run | Time (ms) | HTTP Status |
|-----|-----------|-------------|
| 1 | 1687 | 200 |
| 2 | 4407 | 200 |
| 3 | 1641 | 200 |

- **Median**: 1687ms | **Min**: 1641ms | **Max**: 4407ms
- **Tools invoked**: none
- **Response**: `As an AI without real-time access, I can't provide today's specific news updates`

### grok-4-fast | responses_web_search | simple

| Run | Time (ms) | HTTP Status |
|-----|-----------|-------------|
| 1 | 1790 | 200 |
| 2 | 1389 | 200 |
| 3 | 1225 | 200 |

- **Median**: 1389ms | **Min**: 1225ms | **Max**: 1790ms
- **Tools invoked**: none
- **Response**: `Why did the scarecrow win an award? Because he was outstanding in his field!`

### grok-4-fast | responses_web_search | search

| Run | Time (ms) | HTTP Status |
|-----|-----------|-------------|
| 1 | 5752 | 200 |
| 2 | 7445 | 200 |
| 3 | 10138 | 200 |

- **Median**: 7445ms | **Min**: 5752ms | **Max**: 10138ms
- **Tools invoked**: web_search_call, web_search_call
- **Response**: `Today, world leaders gathered at the Munich Security Conference amid concerns ov`

### grok-4-fast | chat_completions | simple

| Run | Time (ms) | HTTP Status |
|-----|-----------|-------------|
| 1 | 794 | 200 |
| 2 | 665 | 200 |
| 3 | 1018 | 200 |

- **Median**: 794ms | **Min**: 665ms | **Max**: 1018ms
- **Tools invoked**: none
- **Response**: `Why don't scientists trust atoms? Because they make up everything!`

### grok-4-fast | chat_completions | search

| Run | Time (ms) | HTTP Status |
|-----|-----------|-------------|
| 1 | 4157 | 200 |
| 2 | 2764 | 200 |
| 3 | 3668 | 200 |

- **Median**: 3668ms | **Min**: 2764ms | **Max**: 4157ms
- **Tools invoked**: none
- **Response**: `Major headlines today include escalating tensions in the Middle East with report`


---
## Model: `grok-4`

### grok-4 | responses_no_tools | simple

| Run | Time (ms) | HTTP Status |
|-----|-----------|-------------|
| 1 | 2878 | 200 |
| 2 | 3192 | 200 |
| 3 | 3286 | 200 |

- **Median**: 3192ms | **Min**: 2878ms | **Max**: 3286ms
- **Tools invoked**: none
- **Response**: `Why did the scarecrow win an award? Because he was outstanding in his field!`

### grok-4 | responses_no_tools | search

| Run | Time (ms) | HTTP Status |
|-----|-----------|-------------|
| 1 | 6345 | 200 |
| 2 | 6483 | 200 |
| 3 | 6473 | 200 |

- **Median**: 6473ms | **Min**: 6345ms | **Max**: 6483ms
- **Tools invoked**: none
- **Response**: `I'm an AI without real-time internet access, so I can't provide the latest news `

### grok-4 | responses_web_search | simple

| Run | Time (ms) | HTTP Status |
|-----|-----------|-------------|
| 1 | 4960 | 200 |
| 2 | 5569 | 200 |
| 3 | 4574 | 200 |

- **Median**: 4960ms | **Min**: 4574ms | **Max**: 5569ms
- **Tools invoked**: none
- **Response**: `Why did the scarecrow win an award? Because he was outstanding in his field!`

### grok-4 | responses_web_search | search

| Run | Time (ms) | HTTP Status |
|-----|-----------|-------------|
| 1 | 27894 | 200 |
| 2 | 19977 | 200 |
| 3 | 19444 | 200 |

- **Median**: 19977ms | **Min**: 19444ms | **Max**: 27894ms
- **Tools invoked**: web_search_call
- **Response**: `Today's top news headlines include former President Trump threatening to block a`

### grok-4 | chat_completions | simple

| Run | Time (ms) | HTTP Status |
|-----|-----------|-------------|
| 1 | 4139 | 200 |
| 2 | 3572 | 200 |
| 3 | 3029 | 200 |

- **Median**: 3572ms | **Min**: 3029ms | **Max**: 4139ms
- **Tools invoked**: none
- **Response**: `Why don't scientists trust atoms? Because they make up everything!`

### grok-4 | chat_completions | search

| Run | Time (ms) | HTTP Status |
|-----|-----------|-------------|
| 1 | 7576 | 200 |
| 2 | 9637 | 200 |
| 3 | 5542 | 200 |

- **Median**: 7576ms | **Min**: 5542ms | **Max**: 9637ms
- **Tools invoked**: none
- **Response**: `I don't have real-time access to current events, so I can't provide today's news`

---

## Full Results Matrix

| Model | Config | Simple (ms) | Search (ms) | Errors |
|-------|--------|-------------|-------------|--------|
| `grok-4-1-fast` | No tools | 1613 | 5387 | 0 |
| `grok-4-1-fast` | + web_search | 1883 | 20453 | 0 |
| `grok-4-1-fast` | Chat Completions | 1489 | 5374 | 0 |
| `grok-4-fast` | No tools | 900 | 1687 | 0 |
| `grok-4-fast` | + web_search | 1389 | 7445 | 0 |
| `grok-4-fast` | Chat Completions | 794 | 3668 | 0 |
| `grok-4` | No tools | 3192 | 6473 | 0 |
| `grok-4` | + web_search | 4960 | 19977 | 0 |
| `grok-4` | Chat Completions | 3572 | 7576 | 0 |

## Speed Ranking

Ranked by **Responses API, no tools, simple prompt** (most common smartwatch interaction):

| Rank | Model | Median (ms) | vs Current (`grok-4-1-fast`) |
|------|-------|-------------|-------------------------------|
| 1 | `grok-4-fast` | 900 | -44% |
| 2 | `grok-4-1-fast` | 1613 | baseline |
| 3 | `grok-4` | 3192 | +98% |

## Recommendation

**Fastest model for smartwatch use: `grok-4-fast`** (900ms median for simple queries)

- Simple queries (no tools): ~900ms
- Search queries (web_search): ~7445ms

### To apply this change, update:

1. `src/pkjs/index.js` line 99: `TEST_MODEL = 'grok-4-fast'`
2. `src/pkjs/index.js` line 534: `defaultModel = 'grok-4-fast'`
3. `src/pkjs/index.js` line 707: List `grok-4-fast` first with "(fastest)"
4. `config/config.js` line 19: `model: 'grok-4-fast'`
