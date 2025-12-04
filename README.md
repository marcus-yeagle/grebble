# Grok for Pebble

Chat with Grok AI directly from your Pebble smartwatch. This app is unaffiliated with xAI and was made by independent developers as an open-source initiative.

![Grok for Pebble](screenshots/preview.png)

## Features

- **Voice Input** — Use Pebble's built-in voice dictation to send messages to Grok
- **Real-time Streaming** — Receive responses from Grok as they're generated
- **Conversation History** — Maintains context throughout your conversation with scrollable message history
- **Animated Grok Pulse** — Features a pulsing X animation while waiting for responses
- **Dark Theme** — Space-themed UI with black backgrounds and electric blue accents
- **Configurable** — Customize API endpoint, model selection, and system prompts

## Screenshots

### Basalt (Pebble Time)
<p>
<img src="screenshots/basalt-1.png" width="168" alt="Basalt - Welcome Screen" />
<img src="screenshots/basalt-2.png" width="168" alt="Basalt - Chat Interface" />
<img src="screenshots/basalt-3.png" width="168" alt="Basalt - Loading" />
</p>

### Diorite (Pebble 2)
<p>
<img src="screenshots/diorite-1.png" width="168" alt="Diorite - Welcome Screen" />
<img src="screenshots/diorite-2.png" width="168" alt="Diorite - Chat Interface" />
<img src="screenshots/diorite-3.png" width="168" alt="Diorite - Loading" />
</p>

## Requirements

- A Pebble smartwatch (Pebble, Pebble Steel, Pebble Time, Pebble Time Steel, Pebble Time Round, Pebble 2)
- Rebble services configured on your phone
- An xAI API key (get one at [x.ai/api](https://x.ai/api))

## Installation

### Option 1: Install from Rebble App Store
*(Coming soon)*

### Option 2: Sideload
1. Download the latest `.pbw` file from the [Releases](https://github.com/yourusername/grok-for-pebble/releases) page
2. Open the file on your phone with the Pebble app installed
3. The app will be installed on your watch

### Option 3: Build from Source
See [Building from Source](#building-from-source) below.

## Setup

1. Install the app on your Pebble watch
2. Open the Pebble app on your phone
3. Go to the app settings for "Grok"
4. Enter your xAI API key
5. (Optional) Configure model and system message

## Usage

1. Launch the app on your Pebble
2. Press the middle (select) button to start voice dictation
3. Speak your message
4. Wait for Grok's response
5. Scroll with up/down buttons to read the conversation
6. Press select again to continue the conversation

## Configuration Options

| Setting | Default | Description |
|---------|---------|-------------|
| API Key | (required) | Your xAI API key from x.ai/api |
| Base URL | `https://api.x.ai/v1/messages` | API endpoint. Use `/v1/chat/completions` for OpenAI-compatible format |
| Model | `grok-3-mini` | Model to use. Options: `grok-3-mini`, `grok-3`, `grok-4` |
| System Message | Pebble-optimized prompt | Customize Grok's behavior and personality |

## Building from Source

### Prerequisites
- [Pebble SDK](https://developer.rebble.io/developer.pebble.com/sdk/index.html) (via Rebble)
- Node.js (for JS dependencies)

### Build Steps
```bash
# Clone the repository
git clone https://github.com/yourusername/grok-for-pebble.git
cd grok-for-pebble

# Install dependencies
npm install

# Build for all platforms
pebble build

# Install on your watch (with phone connected)
pebble install --phone YOUR_PHONE_IP
```

### Build for Emulator
```bash
pebble build
pebble install --emulator basalt
```

## API Details

This app uses xAI's Grok API. Two endpoint formats are supported:

### Anthropic-compatible (default)
```
POST https://api.x.ai/v1/messages
Authorization: Bearer YOUR_API_KEY
```

### OpenAI-compatible
```
POST https://api.x.ai/v1/chat/completions
Authorization: Bearer YOUR_API_KEY
```

The app automatically detects which format to use based on the URL.

## Troubleshooting

### "No API key configured"
Open the Pebble app on your phone, go to the Grok app settings, and enter your xAI API key.

### Voice dictation not working
Make sure Rebble services are properly configured on your phone. Voice dictation requires an active internet connection.

### Responses are slow
Grok models, especially `grok-4`, can take time to generate responses due to reasoning. Consider using `grok-3-mini` for faster responses.

### Network errors
Ensure your phone has an active internet connection and the xAI API is accessible.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## Credits

- Inspired by and adapted from [Claude for Pebble](https://github.com/breitburg/claude-for-pebble) by Ilia Breitburg
- xAI for creating Grok
- The Rebble community for keeping Pebble alive

## License

This project is licensed under the Unlicense - see the [LICENSE](LICENSE) file for details.

## Disclaimer

This app is not affiliated with, endorsed by, or sponsored by xAI. "Grok" and the xAI name are trademarks of xAI. This is an independent, open-source project created by enthusiasts.

