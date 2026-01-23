/**
 * Grebble - Phone-side JavaScript
 * 
 * Handles API communication with xAI's Grok API and
 * configuration management.
 * 
 * Uses the Anthropic-compatible endpoint for easier migration.
 */

// PebbleKit JS (older SDK runtimes) may not provide `fetch`. The debug-mode
// instrumentation below uses fetch() as required, so we polyfill it using XHR.
if (typeof fetch !== 'function') {
  // eslint-disable-next-line no-var
  var fetch = function (url, options) {
    try {
      options = options || {};
      var xhr = new XMLHttpRequest();
      xhr.open(options.method || 'GET', url, true);
      if (options.headers) {
        for (var k in options.headers) {
          if (Object.prototype.hasOwnProperty.call(options.headers, k)) {
            xhr.setRequestHeader(k, options.headers[k]);
          }
        }
      }
      xhr.send(options.body || null);
    } catch (e) {
      // swallow
    }
    return { catch: function () {} };
  };
}

// Parse encoded conversation string "[U]msg1[A]msg2..." into messages array
function parseConversation(encoded) {
  var messages = [];
  var parts = encoded.split(/(\[U\]|\[A\])/);

  var currentRole = null;
  for (var i = 0; i < parts.length; i++) {
    if (parts[i] === '[U]') {
      currentRole = 'user';
    } else if (parts[i] === '[A]') {
      currentRole = 'assistant';
    } else if (parts[i] && parts[i].length > 0 && currentRole) {
      messages.push({
        role: currentRole,
        content: parts[i]
      });
      currentRole = null;
    }
  }

  return messages;
}

// Test configuration for emulator development (remove before production!)
var TEST_API_KEY = '[REDACTED-API-KEY]';
var TEST_BASE_URL = 'https://api.x.ai/v1/chat/completions';
var TEST_MODEL = 'grok-4-1-fast-reasoning';
var TEST_SYSTEM = 'Respond succinctly in 1-3 sentences max.';

// Get response from Grok API (xAI)
function getGrokResponse(messages) {
  var apiKey = localStorage.getItem('api_key') || TEST_API_KEY;
  var baseUrl = localStorage.getItem('base_url') || TEST_BASE_URL;
  var model = localStorage.getItem('model') || TEST_MODEL;
  var defaultSystem = TEST_SYSTEM;
  var systemMessage = localStorage.getItem('system_message') || defaultSystem;

  if (!apiKey) {
    console.log('No API key configured');
    Pebble.sendAppMessage({ 'RESPONSE_TEXT': 'No API key configured. Please configure in settings.' });
    Pebble.sendAppMessage({ 'RESPONSE_END': 1 });
    return;
  }

  console.log('Sending request to Grok API with ' + messages.length + ' messages');

  var xhr = new XMLHttpRequest();
  xhr.open('POST', baseUrl, true);
  xhr.setRequestHeader('Content-Type', 'application/json');
  xhr.setRequestHeader('Authorization', 'Bearer ' + apiKey);

  var isOpenAIFormat = baseUrl.indexOf('/chat/completions') !== -1;
  
  xhr.timeout = 30000;

  xhr.onload = function () {
    // #region agent log
    fetch('http://127.0.0.1:7245/ingest/3d3415d5-02b7-4cb3-8e49-bea855146955',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({sessionId:'debug-session',runId:'pre-fix',hypothesisId:'H4',location:'src/pkjs/index.js:xhr.onload',message:'Grok API response received',data:{status:xhr.status,respLen:(xhr.responseText||'').length,baseUrlIsOpenAI:isOpenAIFormat},timestamp:Date.now()})}).catch(()=>{});
    // #endregion
    if (xhr.status === 200) {
      try {
        var data = JSON.parse(xhr.responseText);
        var responseText = '';

        if (isOpenAIFormat) {
          if (data.choices && data.choices.length > 0 && data.choices[0].message) {
            responseText = data.choices[0].message.content || '';
          }
        } else {
          if (data.content && data.content.length > 0) {
            for (var i = 0; i < data.content.length; i++) {
              var block = data.content[i];
              if (block.type === 'text' && block.text) {
                responseText += block.text;
              }
            }
          }
        }

        responseText = responseText.trim();

        if (responseText.length > 0) {
          console.log('Sending response: ' + responseText);
          Pebble.sendAppMessage(
            { 'RESPONSE_TEXT': responseText },
            function () {},
            function (e) {
              // #region agent log
              fetch('http://127.0.0.1:7245/ingest/3d3415d5-02b7-4cb3-8e49-bea855146955',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({sessionId:'debug-session',runId:'pre-fix',hypothesisId:'H2',location:'src/pkjs/index.js:sendAppMessage(RESPONSE_TEXT):failure',message:'RESPONSE_TEXT failed to deliver to watch',data:{responseLen:responseText.length,error:(e&&e.error)?{message:e.error.message,code:e.error.code}:null},timestamp:Date.now()})}).catch(()=>{});
              // #endregion
            }
          );
        } else {
          console.log('No text in response');
          Pebble.sendAppMessage({ 'RESPONSE_TEXT': 'No response from Grok' });
        }
      } catch (e) {
        console.log('Error parsing response: ' + e);
        Pebble.sendAppMessage({ 'RESPONSE_TEXT': 'Error parsing response' });
      }
    } else {
      console.log('API error: ' + xhr.status + ' - ' + xhr.responseText);
      var errorMessage = xhr.responseText;

      try {
        var errorData = JSON.parse(xhr.responseText);
        if (errorData.error && errorData.error.message) {
          errorMessage = errorData.error.message;
        } else if (errorData.message) {
          errorMessage = errorData.message;
        }
      } catch (e) {
        console.log('Failed to parse error response: ' + e);
      }

      if (errorMessage.length > 200) {
        errorMessage = errorMessage.substring(0, 200) + '...';
      }

      Pebble.sendAppMessage({ 'RESPONSE_TEXT': 'Error ' + xhr.status + ': ' + errorMessage });
    }

    Pebble.sendAppMessage({ 'RESPONSE_END': 1 });
  };

  xhr.onerror = function () {
    console.log('Network error');
    // #region agent log
    fetch('http://127.0.0.1:7245/ingest/3d3415d5-02b7-4cb3-8e49-bea855146955',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({sessionId:'debug-session',runId:'pre-fix',hypothesisId:'H4',location:'src/pkjs/index.js:xhr.onerror',message:'Grok API network error',data:{baseUrl:(baseUrl||'').substring(0,80)},timestamp:Date.now()})}).catch(()=>{});
    // #endregion
    Pebble.sendAppMessage({ 'RESPONSE_TEXT': 'Network error occurred' });
    Pebble.sendAppMessage({ 'RESPONSE_END': 1 });
  };

  xhr.ontimeout = function () {
    console.log('Request timeout');
    // #region agent log
    fetch('http://127.0.0.1:7245/ingest/3d3415d5-02b7-4cb3-8e49-bea855146955',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({sessionId:'debug-session',runId:'pre-fix',hypothesisId:'H4',location:'src/pkjs/index.js:xhr.ontimeout',message:'Grok API request timeout',data:{timeoutMs:xhr.timeout,baseUrl:(baseUrl||'').substring(0,80)},timestamp:Date.now()})}).catch(()=>{});
    // #endregion
    Pebble.sendAppMessage({ 'RESPONSE_TEXT': 'Request timed out. Please try again.' });
    Pebble.sendAppMessage({ 'RESPONSE_END': 1 });
  };

  var requestBody;

  if (isOpenAIFormat) {
    requestBody = {
      model: model,
      max_tokens: 256,
      messages: messages,
      search_parameters: {
        mode: 'auto'  // Enable live web search when Grok deems it helpful
      }
    };

    if (systemMessage) {
      requestBody.messages = [{
        role: 'system',
        content: systemMessage
      }].concat(messages);
    }
  } else {
    requestBody = {
      model: model,
      max_tokens: 256,
      messages: messages,
      search_parameters: {
        mode: 'auto'  // Enable live web search when Grok deems it helpful
      }
    };

    if (systemMessage) {
      requestBody.system = systemMessage;
    }
  }

  console.log('Request body: ' + JSON.stringify(requestBody));
  xhr.send(JSON.stringify(requestBody));
}

function sendReadyStatus() {
  var apiKey = localStorage.getItem('api_key') || TEST_API_KEY;
  var isReady = apiKey && apiKey.trim().length > 0 ? 1 : 0;

  console.log('Sending READY_STATUS: ' + isReady);
  
  // Build message with ready status and canned prompts
  var message = { 'READY_STATUS': isReady };
  
  // Include canned prompts if they exist
  for (var i = 1; i <= 5; i++) {
    var prompt = localStorage.getItem('canned_prompt_' + i);
    if (prompt && prompt.trim().length > 0) {
      message['CANNED_PROMPT_' + i] = prompt.trim();
    }
  }

  // #region agent log
  fetch('http://127.0.0.1:7245/ingest/3d3415d5-02b7-4cb3-8e49-bea855146955',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({sessionId:'debug-session',runId:'pre-fix',hypothesisId:'H1',location:'src/pkjs/index.js:sendReadyStatus',message:'Sending READY_STATUS to watch',data:{isReady:isReady,keys:Object.keys(message).length},timestamp:Date.now()})}).catch(()=>{});
  // #endregion

  Pebble.sendAppMessage(
    message,
    function () {},
    function (e) {
      // #region agent log
      fetch('http://127.0.0.1:7245/ingest/3d3415d5-02b7-4cb3-8e49-bea855146955',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({sessionId:'debug-session',runId:'pre-fix',hypothesisId:'H1',location:'src/pkjs/index.js:sendAppMessage(READY_STATUS):failure',message:'READY_STATUS failed to deliver to watch',data:{isReady:isReady,error:(e&&e.error)?{message:e.error.message,code:e.error.code}:null},timestamp:Date.now()})}).catch(()=>{});
      // #endregion
    }
  );
}

Pebble.addEventListener('ready', function () {
  console.log('PebbleKit JS ready - Grebble');
  var watchInfo = null;
  try {
    if (Pebble.getActiveWatchInfo) {
      watchInfo = Pebble.getActiveWatchInfo();
    }
  } catch (e) {
    watchInfo = { error: '' + e };
  }

  // #region agent log
  fetch('http://127.0.0.1:7245/ingest/3d3415d5-02b7-4cb3-8e49-bea855146955',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({sessionId:'debug-session',runId:'pre-fix',hypothesisId:'H1',location:'src/pkjs/index.js:ready',message:'PebbleKit JS ready fired',data:{hasLocalStorage:typeof localStorage!=='undefined',hasFetch:typeof fetch==='function',watchInfo:watchInfo},timestamp:Date.now()})}).catch(()=>{});
  // #endregion
  sendReadyStatus();
});

Pebble.addEventListener('appmessage', function (e) {
  console.log('Received message from watch');

  if (e.payload.REQUEST_CHAT) {
    var encoded = e.payload.REQUEST_CHAT;
    console.log('REQUEST_CHAT received: ' + encoded);

    // #region agent log
    fetch('http://127.0.0.1:7245/ingest/3d3415d5-02b7-4cb3-8e49-bea855146955',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({sessionId:'debug-session',runId:'pre-fix',hypothesisId:'H3',location:'src/pkjs/index.js:appmessage',message:'REQUEST_CHAT received from watch',data:{encodedLen:(encoded||'').length,payloadKeys:e&&e.payload?Object.keys(e.payload):[]},timestamp:Date.now()})}).catch(()=>{});
    // #endregion

    var messages = parseConversation(encoded);
    console.log('Parsed ' + messages.length + ' messages');

    getGrokResponse(messages);
  }
});

Pebble.addEventListener('showConfiguration', function () {
  var apiKey = localStorage.getItem('api_key') || '';
  var baseUrl = localStorage.getItem('base_url') || '';
  var model = localStorage.getItem('model') || '';
  var systemMessage = localStorage.getItem('system_message') || '';
  
  // Load canned prompts
  var cannedPrompts = [];
  for (var i = 1; i <= 5; i++) {
    cannedPrompts.push(localStorage.getItem('canned_prompt_' + i) || '');
  }

  var configHtml = getConfigPageHtml(apiKey, baseUrl, model, systemMessage, cannedPrompts);
  var url = 'data:text/html,' + encodeURIComponent(configHtml);

  console.log('Opening configuration page');
  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (e && e.response) {
    try {
      var settings = JSON.parse(decodeURIComponent(e.response));
      console.log('Settings received: ' + JSON.stringify(settings));

      // Check if this is a phone message (send to watch directly)
      if (settings.phone_message && settings.phone_message.trim() !== '') {
        console.log('Sending phone message to watch: ' + settings.phone_message);
        Pebble.sendAppMessage({ 'PHONE_MESSAGE': settings.phone_message.trim() });
        return;
      }

      // Handle normal settings
      var keys = ['api_key', 'base_url', 'model', 'system_message'];
      for (var i = 0; i < keys.length; i++) {
        var key = keys[i];
        if (settings[key] && settings[key].trim() !== '') {
          localStorage.setItem(key, settings[key]);
          console.log(key + ' saved');
        } else {
          localStorage.removeItem(key);
          console.log(key + ' cleared');
        }
      }
      
      // Handle canned prompts
      for (var j = 1; j <= 5; j++) {
        var promptKey = 'canned_prompt_' + j;
        if (settings[promptKey] !== undefined) {
          if (settings[promptKey] && settings[promptKey].trim() !== '') {
            localStorage.setItem(promptKey, settings[promptKey].trim());
            console.log(promptKey + ' saved: ' + settings[promptKey].trim());
          } else {
            localStorage.removeItem(promptKey);
            console.log(promptKey + ' cleared');
          }
        }
      }

      sendReadyStatus();
    } catch (err) {
      console.log('Error parsing settings: ' + err);
    }
  }
});

function escapeHtml(text) {
  if (!text) return '';
  return text
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

function getConfigPageHtml(apiKey, baseUrl, model, systemMessage, cannedPrompts) {
  var defaultBaseUrl = 'https://api.x.ai/v1/chat/completions';
  var defaultModel = 'grok-3-mini';
  var defaultSystem = 'You are Grok, a helpful AI built by xAI. Running on a Pebble smartwatch. Respond in plain text, 1-3 sentences. Be witty and concise.';
  var defaultPrompts = ['Hello', "What's the weather?", 'Tell me a joke', 'Thanks!', 'Goodbye'];
  
  // Ensure cannedPrompts array exists
  cannedPrompts = cannedPrompts || ['', '', '', '', ''];
  
  var html = '<!DOCTYPE html><html><head>' +
    '<meta charset="utf-8">' +
    '<meta name="viewport" content="width=device-width, initial-scale=1">' +
    '<title>Grebble Settings</title>' +
    '<style>' +
    '* { box-sizing: border-box; }' +
    'body { background: #000; color: #e5e5e5; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; padding: 20px; max-width: 600px; margin: 0 auto; min-height: 100vh; }' +
    '.header { display: flex; align-items: center; gap: 12px; margin-bottom: 4px; }' +
    '.logo { width: 36px; height: 36px; flex-shrink: 0; }' +
    'h1 { color: #fff; margin: 0; font-size: 28px; font-weight: 600; }' +
    'h2 { color: #fff; font-size: 16px; font-weight: 600; margin: 0 0 8px 0; }' +
    'p { color: #71717a; font-size: 14px; margin: 0; }' +
    'a { color: #fff; text-decoration: underline; }' +
    '.subtitle { margin-bottom: 24px; margin-top: 8px; }' +
    '.section { background: #18181b; border: 1px solid #27272a; border-radius: 12px; padding: 20px; margin-bottom: 16px; }' +
    '.section-desc { color: #71717a; font-size: 14px; margin: 0 0 16px 0; }' +
    '.form-group { margin-bottom: 16px; }' +
    '.form-group:last-child { margin-bottom: 0; }' +
    '.form-group-small { margin-bottom: 12px; }' +
    'label { display: block; color: #a1a1aa; font-size: 13px; margin-bottom: 8px; font-weight: 500; }' +
    'input, textarea { width: 100%; background: #09090b; border: 1px solid #27272a; color: #fff; padding: 12px; border-radius: 8px; font-size: 14px; transition: border-color 0.2s; }' +
    'input::placeholder, textarea::placeholder { color: #52525b; }' +
    'input:focus, textarea:focus { border-color: #52525b; outline: none; box-shadow: 0 0 0 2px rgba(255,255,255,0.05); }' +
    'textarea { font-family: "SF Mono", Monaco, monospace; resize: vertical; min-height: 100px; }' +
    '.hint { font-size: 12px; color: #52525b; margin-top: 6px; }' +
    '.hint a { color: #a1a1aa; }' +
    '.hint code { background: #27272a; padding: 2px 6px; border-radius: 4px; font-size: 11px; }' +
    '.advanced { display: none; }' +
    '.advanced-title { color: #52525b; font-size: 11px; text-transform: uppercase; letter-spacing: 1px; margin-bottom: 16px; font-weight: 600; }' +
    '.buttons { display: flex; gap: 10px; margin-top: 24px; }' +
    'button { flex: 1; padding: 14px 24px; border-radius: 8px; font-size: 15px; font-weight: 600; cursor: pointer; border: none; transition: all 0.15s ease; }' +
    '#save { background: #fff; color: #000; }' +
    '#save:hover { background: #e5e5e5; }' +
    '#save:active { background: #d4d4d4; transform: scale(0.98); }' +
    '#reset { background: transparent; color: #a1a1aa; border: 1px solid #27272a; }' +
    '#reset:hover { background: #18181b; color: #fff; border-color: #3f3f46; }' +
    '#send-msg { background: #fff; color: #000; width: 100%; margin-top: 8px; }' +
    '#send-msg:hover { background: #e5e5e5; }' +
    '#send-msg:active { background: #d4d4d4; transform: scale(0.98); }' +
    '.prompt-input { padding: 10px; font-size: 13px; }' +
    '.prompt-label { font-size: 12px; color: #52525b; margin-bottom: 4px; }' +
    '</style>' +
    '</head><body>' +
    '<div class="header">' +
    '<svg class="logo" viewBox="0 0 150 150" xmlns="http://www.w3.org/2000/svg">' +
    '<path fill="#fff" d="M59.95,93.59l45.05-33.3c2.21-1.63,5.37-1,6.42,1.54,5.54,13.37,3.06,29.44-7.96,40.48-11.02,11.03-26.35,13.45-40.37,7.94l-15.31,7.1c21.96,15.03,48.63,11.31,65.29-5.38,13.22-13.23,17.31-31.27,13.48-47.54l.03,.03c-5.55-23.9,1.36-33.45,15.53-52.98,.33-.46,.67-.93,1.01-1.4l-18.64,18.66v-.06L59.94,93.6"/>' +
    '<path fill="#fff" d="M50.65,101.68c-15.76-15.07-13.04-38.4,.4-51.86,9.94-9.96,26.24-14.02,40.46-8.05l15.28-7.06c-2.75-1.99-6.28-4.13-10.33-5.64-18.29-7.54-40.2-3.79-55.07,11.09-14.3,14.32-18.8,36.34-11.08,55.13,5.77,14.04-3.69,23.98-13.22,34-3.38,3.55-6.76,7.11-9.49,10.87l43.03-38.48"/>' +
    '</svg>' +
    '<h1>Grebble</h1>' +
    '</div>' +
    '<p class="subtitle">Unaffiliated with xAI. Open-source project.</p>' +
    
    // Send Message Section (iOS dictation workaround)
    '<div class="section">' +
    '<h2>Send Message</h2>' +
    '<p class="section-desc">Type a message to send to Grok (useful if voice input is unavailable)</p>' +
    '<div class="form-group">' +
    '<input type="text" id="phone-message" placeholder="Type your message here...">' +
    '</div>' +
    '<button id="send-msg">Send to Watch</button>' +
    '</div>' +
    
    // Quick Replies Section
    '<div class="section">' +
    '<h2>Quick Replies</h2>' +
    '<p class="section-desc">Customize the 5 quick reply options available on your watch. Leave empty to use defaults.</p>' +
    '<div class="form-group-small">' +
    '<label class="prompt-label">Quick Reply 1 (default: ' + defaultPrompts[0] + ')</label>' +
    '<input type="text" class="prompt-input" id="prompt-1" placeholder="' + defaultPrompts[0] + '" value="' + escapeHtml(cannedPrompts[0]) + '">' +
    '</div>' +
    '<div class="form-group-small">' +
    '<label class="prompt-label">Quick Reply 2 (default: ' + defaultPrompts[1] + ')</label>' +
    '<input type="text" class="prompt-input" id="prompt-2" placeholder="' + defaultPrompts[1] + '" value="' + escapeHtml(cannedPrompts[1]) + '">' +
    '</div>' +
    '<div class="form-group-small">' +
    '<label class="prompt-label">Quick Reply 3 (default: ' + defaultPrompts[2] + ')</label>' +
    '<input type="text" class="prompt-input" id="prompt-3" placeholder="' + defaultPrompts[2] + '" value="' + escapeHtml(cannedPrompts[2]) + '">' +
    '</div>' +
    '<div class="form-group-small">' +
    '<label class="prompt-label">Quick Reply 4 (default: ' + defaultPrompts[3] + ')</label>' +
    '<input type="text" class="prompt-input" id="prompt-4" placeholder="' + defaultPrompts[3] + '" value="' + escapeHtml(cannedPrompts[3]) + '">' +
    '</div>' +
    '<div class="form-group-small">' +
    '<label class="prompt-label">Quick Reply 5 (default: ' + defaultPrompts[4] + ')</label>' +
    '<input type="text" class="prompt-input" id="prompt-5" placeholder="' + defaultPrompts[4] + '" value="' + escapeHtml(cannedPrompts[4]) + '">' +
    '</div>' +
    '<div class="hint">These will be shown when dictation fails or when you press SELECT on the watch.</div>' +
    '</div>' +
    
    // API Configuration Section
    '<div class="section">' +
    '<h2>API Configuration</h2>' +
    '<p class="section-desc">Configure your xAI API access</p>' +
    '<div class="form-group">' +
    '<label>xAI API Key</label>' +
    '<input type="text" id="api-key" placeholder="xai-..." value="' + escapeHtml(apiKey) + '">' +
    '<div class="hint">Get your API key from <a href="https://x.ai/api" target="_blank">x.ai/api</a></div>' +
    '</div>' +
    '</div>' +
    
    // Advanced Settings Section
    '<div class="section advanced" id="advanced">' +
    '<div class="advanced-title">Advanced Settings</div>' +
    '<div class="form-group">' +
    '<label>Base URL</label>' +
    '<input type="text" id="base-url" value="' + escapeHtml(baseUrl || defaultBaseUrl) + '">' +
    '<div class="hint">Use <code>/v1/chat/completions</code> for OpenAI-compatible format</div>' +
    '</div>' +
    '<div class="form-group">' +
    '<label>Model</label>' +
    '<input type="text" id="model" value="' + escapeHtml(model || defaultModel) + '">' +
    '<div class="hint">Options: <code>grok-3-mini</code>, <code>grok-3</code>, <code>grok-4</code></div>' +
    '</div>' +
    '<div class="form-group">' +
    '<label>System Message</label>' +
    '<textarea id="system-message">' + escapeHtml(systemMessage || defaultSystem) + '</textarea>' +
    '</div>' +
    '</div>' +
    
    '<div class="buttons">' +
    '<button id="save">Save Settings</button>' +
    '<button id="reset">Reset</button>' +
    '</div>' +
    '<script>' +
    'var apiKeyInput = document.getElementById("api-key");' +
    'var advancedDiv = document.getElementById("advanced");' +
    'function toggleAdvanced() { advancedDiv.style.display = apiKeyInput.value.trim() ? "block" : "none"; }' +
    'toggleAdvanced();' +
    'apiKeyInput.addEventListener("input", toggleAdvanced);' +
    
    // Send message button handler
    'document.getElementById("send-msg").addEventListener("click", function() {' +
    '  var msg = document.getElementById("phone-message").value.trim();' +
    '  if (msg) {' +
    '    document.location = "pebblejs://close#" + encodeURIComponent(JSON.stringify({phone_message: msg}));' +
    '  }' +
    '});' +
    
    // Save settings button handler (includes canned prompts)
    'document.getElementById("save").addEventListener("click", function() {' +
    '  var s = {' +
    '    api_key: apiKeyInput.value.trim(),' +
    '    base_url: document.getElementById("base-url").value.trim(),' +
    '    model: document.getElementById("model").value.trim(),' +
    '    system_message: document.getElementById("system-message").value.trim(),' +
    '    canned_prompt_1: document.getElementById("prompt-1").value.trim(),' +
    '    canned_prompt_2: document.getElementById("prompt-2").value.trim(),' +
    '    canned_prompt_3: document.getElementById("prompt-3").value.trim(),' +
    '    canned_prompt_4: document.getElementById("prompt-4").value.trim(),' +
    '    canned_prompt_5: document.getElementById("prompt-5").value.trim()' +
    '  };' +
    '  document.location = "pebblejs://close#" + encodeURIComponent(JSON.stringify(s));' +
    '});' +
    
    // Reset button handler (clears everything including prompts)
    'document.getElementById("reset").addEventListener("click", function() {' +
    '  document.location = "pebblejs://close#" + encodeURIComponent(JSON.stringify({' +
    '    api_key:"",base_url:"",model:"",system_message:"",' +
    '    canned_prompt_1:"",canned_prompt_2:"",canned_prompt_3:"",canned_prompt_4:"",canned_prompt_5:""' +
    '  }));' +
    '});' +
    '</script>' +
    '</body></html>';
  
  return html;
}
