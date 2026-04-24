# bvi

A C++ program that renders a Bible verse reference to a JPEG image with auto-fitted text. The verse is centered on the canvas with a citation line below.

## Features

- Auto-sizes font to fill the image — longer passages use a smaller font, short verses use a larger one
- Text centered horizontally and vertically on the canvas
- Citation line with configurable style, placement, and size
- Support for multiple Bible translations:
  - **KJV** (King James Version) — default
  - **BSB** (Berean Standard Bible)
  - **WEB** (World English Bible)
- Verse ranges (`John 3:16-17`), verse-to-end-of-chapter (`Romans 8:28-`)
- Configurable image size (default 1920×1080)
- Configurable colors for background, verse text, and citation
- Photo background support with adjustable dimming for text readability
- Optional curly quotation marks around the verse text
- Citation style: em-dash prefix, parentheses-wrapped, or plain (`--citestyle`)
- Citation placement: fixed at bottom edge or attached just below verse text (`--citeplacement`)
- Citation font size: absolute (`--citesize`) or relative to auto (`--citescale`)
- Option to omit the Bible version from the citation (`--citebibleversion=no`)
- Verse text size override: cap at an absolute point size (`--textsize`) or scale relative to the auto-fit (`--textscale`)
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

## Background

### Solid color

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

### Photo background

Use any JPEG or PNG as the background. The photo is resized and cropped to fill the canvas. A darkening pass (`--dim`) improves text readability over busy photos.

```bash
./bvi "John 3:16" --bgphoto="sunset.jpg"
./bvi "John 3:16" --bgphoto="photo.jpg" --dim=40
./bvi "John 3:16" --bgphoto="photo.png" --dim=0 --textcolor=white --citecolor=gray80
```

| Option | Default | Description |
|---|---|---|
| `--bgphoto=FILE` | _(none)_ | Background photo; overrides `--bg` |
| `--dim=N` | `50` | Darken photo 0–100% (0 = no dim, 100 = black) |

## Quotation Marks

Add curly `"` `"` quotation marks around the verse text:

```bash
./bvi "John 3:16" --quotes
./bvi "John 3:16" --no-quotes   # explicitly off (default)
```

## Citation

### Style

Control the format of the citation line with `--citestyle`:

| Value | Output |
|---|---|
| `dash` (default) | `— Philippians 4:6-7 (KJV)` |
| `parens` | `(Philippians 4:6-7, KJV)` |
| `plain` | `Philippians 4:6-7 (KJV)` |

```bash
./bvi "John 3:16" --citestyle=parens
./bvi "John 3:16" --citestyle=plain
```

Use `--citebibleversion=no` (or `false`) to omit the Bible version entirely:

| With version (default) | Without version |
|---|---|
| `— John 3:16 (KJV)` | `— John 3:16` |
| `(John 3:16, KJV)` | `(John 3:16)` |
| `John 3:16 (KJV)` | `John 3:16` |

```bash
./bvi "John 3:16" --citebibleversion=no
./bvi "John 3:16" --citestyle=plain --citebibleversion=no
```

### Placement

Control where the citation appears with `--citeplacement`:

- `bottom` (default) — fixed near the bottom edge of the image
- `below` — positioned just underneath the verse text block

```bash
./bvi "John 3:16" --citeplacement=below
./bvi "John 3:16" --citeplacement=below --citestyle=plain
```

### Size

By default the citation point size is auto-scaled based on image height (~30pt at 1080p). Use one of these options to change it — they cannot be combined.

**`--citescale=PCT`** — scale the auto-calculated size by a percentage:

```bash
./bvi "John 3:16" --citescale=75    # ~75% of auto size
./bvi "John 3:16" --citescale=150   # larger than default
```

**`--citesize=N`** — set an absolute point size:

```bash
./bvi "John 3:16" --citesize=40
./bvi "John 3:16" --width=1080 --height=1080 --citesize=24
```

## Verse Text Size

By default the verse text auto-fits to the largest size that fills the canvas. Use one of these options to reduce it — they cannot be combined.

**`--textscale=PCT`** — shrink the text area to a percentage of its default size. The font still auto-fits within the smaller area, so the result is proportionally smaller text:

```bash
./bvi "John 3:16" --textscale=75    # ~75% of normal size
./bvi "John 3:16" --textscale=50    # ~half size
```

**`--textsize=N`** — cap the verse font at a fixed point size. The font auto-fits up to that maximum and won't exceed it:

```bash
./bvi "John 3:16" --textsize=72
./bvi "Philippians 4:6-7" --textsize=48
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
./bvi --bg="#1a1a2e" --textcolor="#e8e8f0" --citecolor="#8888bb" \
      --citestyle=plain --citeplacement=below --textscale=80 --saveconfig
```

Show current effective settings:
```bash
./bvi --showconfig
```

The `.bvi` file is plain text and can be edited by hand:
```
# bvi configuration
bv               = KJV
width            = 1920
height           = 1080
font             = /System/Library/Fonts/Palatino.ttc
bg               = #1a1a2e
bgphoto          = /path/to/photo.jpg
dim              = 50
textcolor        = #e8e8f0
citecolor        = #8888bb
quotes           = yes
citesize         = 0
citescale        = 100
citestyle        = dash
citeplacement    = bottom
citebibleversion = yes
textsize         = 0
textscale        = 100
```

Supported keys: `bv`, `width`, `height`, `font`, `bg`, `bgphoto`, `dim`, `textcolor`, `citecolor`, `quotes`, `citesize`, `citescale`, `citestyle`, `citeplacement`, `citebibleversion`, `textsize`, `textscale`

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
