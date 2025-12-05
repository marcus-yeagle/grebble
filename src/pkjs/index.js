/**
 * Grok for Pebble - Phone-side JavaScript
 * 
 * Handles API communication with xAI's Grok API and
 * configuration management.
 * 
 * Uses the Anthropic-compatible endpoint for easier migration.
 */

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
var TEST_API_KEY = '';
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
          Pebble.sendAppMessage({ 'RESPONSE_TEXT': responseText });
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
    Pebble.sendAppMessage({ 'RESPONSE_TEXT': 'Network error occurred' });
    Pebble.sendAppMessage({ 'RESPONSE_END': 1 });
  };

  xhr.ontimeout = function () {
    console.log('Request timeout');
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
  
  Pebble.sendAppMessage(message);
}

Pebble.addEventListener('ready', function () {
  console.log('PebbleKit JS ready - Grok for Pebble');
  sendReadyStatus();
});

Pebble.addEventListener('appmessage', function (e) {
  console.log('Received message from watch');

  if (e.payload.REQUEST_CHAT) {
    var encoded = e.payload.REQUEST_CHAT;
    console.log('REQUEST_CHAT received: ' + encoded);

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
    '<title>Grok for Pebble Settings</title>' +
    '<style>' +
    'body { background: #0a0a0a; color: #e0e0e0; font-family: -apple-system, sans-serif; padding: 20px; max-width: 600px; margin: 0 auto; }' +
    'h1 { color: #00bfff; margin-bottom: 5px; }' +
    'h2 { color: #00bfff; font-size: 18px; margin-top: 30px; margin-bottom: 15px; }' +
    'p { color: #888; font-size: 14px; }' +
    'a { color: #00bfff; }' +
    '.form-group { margin-bottom: 20px; }' +
    '.form-group-small { margin-bottom: 12px; }' +
    'label { display: block; color: #aaa; font-size: 14px; margin-bottom: 8px; }' +
    'input, textarea { width: 100%; box-sizing: border-box; background: #1a1a1a; border: 1px solid #333; color: #fff; padding: 12px; border-radius: 8px; font-size: 14px; }' +
    'input:focus, textarea:focus { border-color: #00bfff; outline: none; }' +
    'textarea { font-family: monospace; resize: vertical; min-height: 100px; }' +
    '.hint { font-size: 12px; color: #666; margin-top: 6px; }' +
    '.section { border-top: 1px solid #333; padding-top: 20px; margin-top: 20px; }' +
    '.advanced { display: none; }' +
    '.buttons { margin-top: 20px; }' +
    'button { padding: 14px 24px; border-radius: 8px; font-size: 16px; font-weight: 600; cursor: pointer; border: none; margin-right: 10px; margin-bottom: 10px; }' +
    '#save { background: #00bfff; color: #000; }' +
    '#reset { background: #333; color: #888; }' +
    '#send-msg { background: #00bfff; color: #000; }' +
    '.send-section { background: #111; border-radius: 12px; padding: 20px; margin-bottom: 30px; border: 1px solid #222; }' +
    '.send-section h2 { margin-top: 0; }' +
    '.quick-replies-section { background: #111; border-radius: 12px; padding: 20px; margin-bottom: 30px; border: 1px solid #222; }' +
    '.quick-replies-section h2 { margin-top: 0; }' +
    '.prompt-input { padding: 10px; font-size: 13px; }' +
    '.prompt-label { font-size: 12px; color: #666; margin-bottom: 4px; }' +
    '</style>' +
    '</head><body>' +
    '<h1>Grok for Pebble</h1>' +
    '<p>Unaffiliated with xAI. Open-source project.</p>' +
    
    // Send Message Section (iOS dictation workaround)
    '<div class="send-section">' +
    '<h2>Send Message</h2>' +
    '<p style="margin-bottom: 15px;">Type a message to send to Grok (useful if voice input is unavailable)</p>' +
    '<div class="form-group">' +
    '<input type="text" id="phone-message" placeholder="Type your message here...">' +
    '</div>' +
    '<button id="send-msg">Send to Watch</button>' +
    '</div>' +
    
    // Quick Replies Section
    '<div class="quick-replies-section">' +
    '<h2>Quick Replies</h2>' +
    '<p style="margin-bottom: 15px;">Customize the 5 quick reply options available on your watch. Leave empty to use defaults.</p>' +
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
    
    // Settings Section
    '<h2>Settings</h2>' +
    '<div class="form-group">' +
    '<label>xAI API Key</label>' +
    '<input type="text" id="api-key" placeholder="xai-..." value="' + escapeHtml(apiKey) + '">' +
    '<div class="hint">Get your API key from x.ai/api</div>' +
    '</div>' +
    '<div class="advanced" id="advanced">' +
    '<div class="form-group">' +
    '<label>Base URL</label>' +
    '<input type="text" id="base-url" value="' + escapeHtml(baseUrl || defaultBaseUrl) + '">' +
    '</div>' +
    '<div class="form-group">' +
    '<label>Model</label>' +
    '<input type="text" id="model" value="' + escapeHtml(model || defaultModel) + '">' +
    '<div class="hint">Options: grok-3-mini, grok-3, grok-4</div>' +
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
