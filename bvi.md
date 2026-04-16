# bvi

A C++ program that renders a Bible verse reference to a JPEG image with auto-fitted text. The verse fills the image at the largest font size that fits, centered on the canvas, with a citation line at the bottom.

## Features

- Auto-sizes font to fill the image — longer passages use a smaller font, short verses use a larger one
- Text centered horizontally and vertically on the canvas
- Citation line at the bottom (e.g. `— John 3:16 (KJV)`)
- Support for multiple Bible translations:
  - **KJV** (King James Version) — default
  - **BSB** (Berean Standard Bible)
  - **WEB** (World English Bible)
- Verse ranges (`John 3:16-17`), verse-to-end-of-chapter (`Romans 8:28-`)
- Configurable image size (default 1920×1080)
- Configurable colors for background, verse text, and citation
- Optional curly quotation marks around the verse text
- Configurable citation font size (or auto-scaled based on image height)
- Custom font support
- Config file (`.bvi`) stores per-folder defaults for version, size, colors, and style
- Default output filename derived from the reference (e.g. `John_3_16.jpg`)
- Auto-prompts to download the Bible translation file if not found

## Building

**macOS / Linux (g++):**
```bash
g++ -std=c++11 -o bvi bvi.cpp
```

**Windows (MSVC — run in Developer Command Prompt):**
```bat
cl /EHsc /std:c++17 bvi.cpp /Fe:bvi.exe
```

## Requirements

- C++11 or later
- ImageMagick 7: `brew install imagemagick`
- `curl` (for automatic Bible file download)

## Usage

```bash
./bvi "John 3:16"
./bvi "Philippians 4:6-7"
./bvi "Romans 8:28-"
```

Specify a Bible version:
```bash
./bvi "John 3:16" -bv=BSB
./bvi "John 3:16" --bibleversion=WEB
```

Specify an output file:
```bash
./bvi "John 3:16" --output=john316.jpg
```

Change image dimensions:
```bash
./bvi "John 3:16" --width=1280 --height=720
./bvi "John 3:16" --width=1080 --height=1080
```

## Colors

Colors accept any value ImageMagick understands: named colors, hex codes, or ImageMagick built-ins.

```bash
./bvi "John 3:16" --bg=black --textcolor=white --citecolor=gray60
./bvi "John 3:16" --bg="#1a1a2e" --textcolor="#e8e8f0" --citecolor="#8888bb"
./bvi "John 3:16" --bg=navy --textcolor=gold --citecolor=lightyellow
```

| Option | Default | Description |
|---|---|---|
| `--bg=COLOR` | `black` | Background color |
| `--textcolor=COLOR` | `white` | Verse text color |
| `--citecolor=COLOR` | `gray60` | Citation line color |

## Quotation Marks

Add curly `"` `"` quotation marks around the verse text:

```bash
./bvi "John 3:16" --quotes
./bvi "John 3:16" --no-quotes   # explicitly off (default)
```

## Citation Size

By default the citation point size is auto-scaled based on image height (~30pt at 1080p). Override with a fixed size:

```bash
./bvi "John 3:16" --citesize=40
./bvi "John 3:16" --width=1080 --height=1080 --citesize=24
```

## Font

The default font is `Palatino` (macOS), `Palatino Linotype` (Windows), or `DejaVu-Serif` (Linux). Override with a font name or file path:

```bash
./bvi "John 3:16" --font="Georgia"
./bvi "John 3:16" --font="/Library/Fonts/MyFont.ttf"
```

To list fonts available to ImageMagick:
```bash
magick -list font
```

## Config File (.bvi)

Run `--saveconfig` to save the current settings as defaults for the current folder. Any subsequent `bvi` run in that folder will use those values automatically. Command-line arguments always override the config.

Save current settings as defaults:
```bash
./bvi --bg="#1a1a2e" --textcolor="#e8e8f0" --citecolor="#8888bb" -bv=KJV --quotes --citesize=32 --saveconfig
```

Show current effective settings:
```bash
./bvi --showconfig
```

The `.bvi` file is plain text and can be edited by hand:
```
# bvi configuration
bv        = KJV
width     = 1920
height    = 1080
font      = /System/Library/Fonts/Palatino.ttc
bg        = #1a1a2e
textcolor = #e8e8f0
citecolor = #8888bb
quotes    = yes
citesize  = 32
```

Supported keys: `bv`, `width`, `height`, `font`, `bg`, `textcolor`, `citecolor`, `quotes`, `citesize`

## Bible Translations

- **KJV**: King James Version — `BibleKJV.txt` (downloaded from https://openbible.com/textfiles/kjv.txt)
- **BSB**: Berean Standard Bible — `BibleBSB.txt` (downloaded from https://bereanbible.com/bsb.txt)
- **WEB**: World English Bible — `BibleWEB.txt` (downloaded from https://openbible.com/textfiles/web.txt)

If a Bible translation file is not found, the program will prompt you to download it automatically using `curl`.

## Files

- `bvi.cpp` — Source code
- `BibleKJV.txt` — KJV Bible text (shared with gospel, downloaded on first use)
- `BibleBSB.txt` — BSB Bible text (shared with gospel, downloaded on first use)
- `BibleWEB.txt` — WEB Bible text (shared with gospel, downloaded on first use)
- `.bvi` — Optional per-folder config file (created by `--saveconfig`)
