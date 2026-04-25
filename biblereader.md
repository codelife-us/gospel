# biblereader

A C++ program that opens a full Bible reader in your browser. Click any verse to copy its reference (or text) to the clipboard. The last selected reference is printed to stdout when you quit, making it composable with other tools.

## Features

- Collapsible book/chapter sidebar for navigation
- Full verse text with verse numbers, flowing paragraph style
- Prev/Next chapter navigation (buttons and keyboard shortcuts)
- Full-text search across all verses with highlighted matches
- Click a verse to copy its reference to the clipboard
- Live Bible version switching (KJV ↔ BSB ↔ WEB) while running — no restart needed
- `--verse` mode copies verse text instead of reference
- Last selected reference printed to stdout on exit, for piping to other tools

## Building

```bash
g++ -std=c++11 -o biblereader biblereader.cpp
```

## Usage

```bash
./biblereader
./biblereader -bv=BSB
./biblereader --bibleversion=WEB
./biblereader --verse          # copy verse text instead of reference
./biblereader --port=8080      # use a different port (default 7778)
```

## Options

| Option | Description |
|--------|-------------|
| `-bv=VERSION` | Starting Bible version: `KJV` (default), `BSB`, `WEB` |
| `--bibleversion=VERSION` | Same as `-bv=` |
| `--verse`, `-v` | Copy verse text to clipboard instead of reference |
| `--port=N` | HTTP port for local server (default `7778`) |
| `-h`, `--help` | Show help |

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| ← / Alt+↑ | Previous chapter |
| → / Alt+↓ | Next chapter |

## Version Switching

If multiple Bible files are available (in the current directory or `$HOME`), version selector buttons appear in the top-right of the header. Clicking a version loads it in the background without restarting; subsequent switches use a cache so each file is fetched only once per session.

## Composing with bvi

Use `biblereader` with `bvi` to pick a verse in the browser and render it as an image:

```bash
./bvi "$(./biblereader)"
```

The reference printed on exit is passed directly to `bvi`.

To copy verse text and use it as image text:

```bash
./bvi --text="$(./biblereader --verse)"
```

## Bible Translations

- **KJV**: King James Version — `BibleKJV.txt` (downloaded from https://openbible.com/textfiles/kjv.txt)
- **BSB**: Berean Standard Bible — `BibleBSB.txt` (downloaded from https://bereanbible.com/bsb.txt)
- **WEB**: World English Bible — `BibleWEB.txt` (downloaded from https://openbible.com/textfiles/web.txt)

If a Bible file is not found, the program will prompt you to download it automatically using `curl`.
Bible files are looked up in the current directory first, then `$HOME`.

## Files

- `biblereader.cpp` — Source code
- `BibleKJV.txt` — KJV Bible text (shared with other tools)
- `BibleBSB.txt` — BSB Bible text
- `BibleWEB.txt` — WEB Bible text
