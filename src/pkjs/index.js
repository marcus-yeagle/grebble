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

// Get response from Grok API (xAI)
function getGrokResponse(messages) {
  var apiKey = localStorage.getItem('api_key');
  var baseUrl = localStorage.getItem('base_url') || 'https://api.x.ai/v1/messages';
  var model = localStorage.getItem('model') || 'grok-3-mini';
  var defaultSystem = "You are Grok, a helpful AI built by xAI. Running on a Pebble smartwatch. Respond in plain text, 1-3 sentences. Be witty and concise.";
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
      messages: messages
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
      messages: messages
    };

    if (systemMessage) {
      requestBody.system = systemMessage;
    }
  }

  console.log('Request body: ' + JSON.stringify(requestBody));
  xhr.send(JSON.stringify(requestBody));
}

function sendReadyStatus() {
  var apiKey = localStorage.getItem('api_key');
  var isReady = apiKey && apiKey.trim().length > 0 ? 1 : 0;

  console.log('Sending READY_STATUS: ' + isReady);
  Pebble.sendAppMessage({ 'READY_STATUS': isReady });
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

  var configHtml = getConfigPageHtml(apiKey, baseUrl, model, systemMessage);
  var url = 'data:text/html,' + encodeURIComponent(configHtml);

  console.log('Opening configuration page');
  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (e && e.response) {
    try {
      var settings = JSON.parse(decodeURIComponent(e.response));
      console.log('Settings received: ' + JSON.stringify(settings));

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

function getConfigPageHtml(apiKey, baseUrl, model, systemMessage) {
  var defaultBaseUrl = 'https://api.x.ai/v1/messages';
  var defaultModel = 'grok-3-mini';
  var defaultSystem = 'You are Grok, a helpful AI built by xAI. Running on a Pebble smartwatch. Respond in plain text, 1-3 sentences. Be witty and concise.';
  
  var html = '<!DOCTYPE html><html><head>' +
    '<meta charset="utf-8">' +
    '<meta name="viewport" content="width=device-width, initial-scale=1">' +
    '<title>Grok for Pebble Settings</title>' +
    '<style>' +
    'body { background: #0a0a0a; color: #e0e0e0; font-family: -apple-system, sans-serif; padding: 20px; max-width: 600px; margin: 0 auto; }' +
    'h1 { color: #00bfff; margin-bottom: 5px; }' +
    'p { color: #888; font-size: 14px; }' +
    'a { color: #00bfff; }' +
    '.form-group { margin-bottom: 20px; }' +
    'label { display: block; color: #aaa; font-size: 14px; margin-bottom: 8px; }' +
    'input, textarea { width: 100%; box-sizing: border-box; background: #1a1a1a; border: 1px solid #333; color: #fff; padding: 12px; border-radius: 8px; font-size: 14px; }' +
    'input:focus, textarea:focus { border-color: #00bfff; outline: none; }' +
    'textarea { font-family: monospace; resize: vertical; min-height: 100px; }' +
    '.hint { font-size: 12px; color: #666; margin-top: 6px; }' +
    '.advanced { display: none; border-top: 1px solid #333; padding-top: 20px; margin-top: 20px; }' +
    '.buttons { margin-top: 30px; }' +
    'button { padding: 14px 24px; border-radius: 8px; font-size: 16px; font-weight: 600; cursor: pointer; border: none; margin-right: 10px; }' +
    '#save { background: #00bfff; color: #000; }' +
    '#reset { background: #333; color: #888; }' +
    '</style>' +
    '</head><body>' +
    '<h1>Grok for Pebble</h1>' +
    '<p>Unaffiliated with xAI. Open-source project.</p>' +
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
    '<button id="save">Save</button>' +
    '<button id="reset">Reset</button>' +
    '</div>' +
    '<script>' +
    'var apiKeyInput = document.getElementById("api-key");' +
    'var advancedDiv = document.getElementById("advanced");' +
    'function toggleAdvanced() { advancedDiv.style.display = apiKeyInput.value.trim() ? "block" : "none"; }' +
    'toggleAdvanced();' +
    'apiKeyInput.addEventListener("input", toggleAdvanced);' +
    'document.getElementById("save").addEventListener("click", function() {' +
    '  var s = {' +
    '    api_key: apiKeyInput.value.trim(),' +
    '    base_url: document.getElementById("base-url").value.trim(),' +
    '    model: document.getElementById("model").value.trim(),' +
    '    system_message: document.getElementById("system-message").value.trim()' +
    '  };' +
    '  document.location = "pebblejs://close#" + encodeURIComponent(JSON.stringify(s));' +
    '});' +
    'document.getElementById("reset").addEventListener("click", function() {' +
    '  document.location = "pebblejs://close#" + encodeURIComponent(JSON.stringify({api_key:"",base_url:"",model:"",system_message:""}));' +
    '});' +
    '</script>' +
    '</body></html>';
  
  return html;
}
