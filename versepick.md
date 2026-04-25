# versepick

A C++ terminal program for interactively browsing the Bible by book, chapter, and verse using arrow keys. Selecting a verse copies its reference (or text) to the clipboard.

## Features

- Arrow-key navigation through books, chapters, and verses
- Verse list shows auto-truncated verse text alongside the verse number
- Copies the reference (e.g. `John 3:16`) or verse text to the clipboard on selection
- Returns to browse more after each selection, or quit with `q`
- Support for multiple Bible translations: KJV (default), BSB, WEB

## Building

```bash
g++ -std=c++11 -o versepick versepick.cpp
```

## Usage

```bash
./versepick
./versepick -bv=BSB
./versepick --bibleversion=WEB
./versepick --verse          # copy verse text instead of reference
```

### Navigation

| Key | Action |
|-----|--------|
| ↑ / ↓ | Move selection |
| Enter / → | Select / go deeper |
| ← / Backspace | Go back |
| `q` | Quit |

## Options

| Option | Description |
|--------|-------------|
| `-bv=VERSION` | Bible version: `KJV` (default), `BSB`, `WEB` |
| `--bibleversion=VERSION` | Same as `-bv=` |
| `--verse`, `-v` | Copy verse text to clipboard instead of reference |
| `-h`, `--help` | Show help |

## Composing with bvi

Use `versepick` with `bvi` to pick a verse interactively and render it as an image:

```bash
./bvi "$(./versepick)"
./bvi "$(./versepick -bv=BSB)" -bv=BSB
./bvi "$(./versepick --verse)" --text="$(./versepick --verse)"
```

## Bible Translations

- **KJV**: King James Version — `BibleKJV.txt` (downloaded from https://openbible.com/textfiles/kjv.txt)
- **BSB**: Berean Standard Bible — `BibleBSB.txt` (downloaded from https://bereanbible.com/bsb.txt)
- **WEB**: World English Bible — `BibleWEB.txt` (downloaded from https://openbible.com/textfiles/web.txt)

If a Bible file is not found, the program will prompt you to download it automatically using `curl`.
Bible files are looked up in the current directory first, then `$HOME`.

## Files

- `versepick.cpp` — Source code
- `BibleKJV.txt` — KJV Bible text (shared with other tools)
- `BibleBSB.txt` — BSB Bible text
- `BibleWEB.txt` — WEB Bible text
