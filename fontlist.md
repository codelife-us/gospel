# fontlist

An interactive font browser for macOS. Scans system font directories, opens a browser-based preview page served via a local HTTP server, and prints the selected font path to stdout on exit.

## Building

```bash
g++ -std=c++17 -o fontlist fontlist.cpp
```

## Usage

Open font browser with default sample text:
```bash
./fontlist
```

Customize the sample text:
```bash
./fontlist --sample="For God so loved the world"
./fontlist --sample="Abcdefg 1234"
```

Use a different port (default: 7777):
```bash
./fontlist --port=8080
```

Show help:
```bash
./fontlist -h
./fontlist --help
```

## How It Works

1. Scans `/System/Library/Fonts`, `/Library/Fonts`, and `~/Library/Fonts` for `.ttf`, `.ttc`, and `.otf` files
2. Creates a temporary directory with symlinks to all font files
3. Starts a local Python HTTP server in that directory
4. Opens `http://localhost:7777/` in the default browser
5. Displays a searchable grid of font cards, each rendered in its own font
6. On exit (Enter key), prints the last-clicked font path to stdout and cleans up

## Browser Interface

- **Search box** — filters font cards by name in real time
- **Sample text box** — changes the preview text across all cards
- **Click a card** — highlights it, copies the full path to clipboard, and shows it in the status bar at the bottom
- **Status bar** — shows the currently selected font path

## Printing the Selected Path

Press **Enter** in the terminal to quit. The path of the last-clicked font is printed to stdout:

```
/System/Library/Fonts/Palatino.ttc
```

This makes it easy to pipe into other tools:

```bash
./bvi "John 3:16" --font="$(./fontlist)"
```

```bash
./fontlist | pbcopy
```

## Requirements

- C++17 or later
- `python3` (available by default on macOS)
- A browser (opened automatically via `open`)
