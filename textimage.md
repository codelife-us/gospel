# textimage

A C++ program that renders plain text to a JPEG image with auto-fitted text. The text is centered on the canvas with no citation line.

## Features

- Auto-sizes font to fill the image — longer text uses a smaller font, short text uses a larger one
- Text centered horizontally and vertically on the canvas
- Hard line breaks via `\n` in the text argument
- Configurable image size (default 1920×1080)
- Configurable colors for background and text
- Photo background support with adjustable dimming for text readability
- Text size: force an absolute point size (`--textsize`), cap auto-fit at a maximum (`--maxtextsize`), or scale relative to auto-fit (`--textscale`)
- Semi-transparent panel behind text for readability (`--textpanel`)
- Drop shadow behind text with adjustable intensity (`--textshadow[=N]`)
- Outline around text for stronger emphasis (`--textoutline[=N]`, `--textoutlinecolor`)
- Shadow style: soft Gaussian blur or hard offset copy (`--shadowmethod`)
- Adjustable line spacing (`--linespacing`)
- Custom font support
- Config file (`.luminaverse` `[textimage]` section) stores per-folder defaults
- Default output filename derived from the first few words of the text

## Building

**macOS / Linux (g++):**
```bash
g++ -std=c++17 -o textimage textimage.cpp
```

**Windows (MSVC — run in Developer Command Prompt):**
```bat
cl /EHsc /std:c++17 textimage.cpp /Fe:textimage.exe
```

## Requirements

- C++11 or later
- ImageMagick 7: `brew install imagemagick`

## Usage

```bash
./textimage "He is risen."
./textimage "Be still and know that I am God."
./textimage --text="Grace upon grace."
```

Specify an output file:
```bash
./textimage "He is risen." --output=risen.jpg
```

Change image dimensions:
```bash
./textimage "He is risen." --width=1280 --height=720
./textimage "He is risen." --width=1080 --height=1080
```

## Line Breaks

Use `\n` in the text to force a line break at that point. ImageMagick's `caption:` operator treats embedded newlines as hard line breaks, overriding its automatic word-wrap.

```bash
./textimage "He is risen.\nDeath has no power."
./textimage "Line one\nLine two\nLine three"
./textimage "Verse one\n\nVerse two"   # double \n = blank line between stanzas
```

## Background

### Solid color

Colors accept any value ImageMagick understands: named colors, hex codes, or ImageMagick built-ins.

```bash
./textimage "He is risen." --bg=black --textcolor=white
./textimage "He is risen." --bg="#1a1a2e" --textcolor="#e8e8f0"
./textimage "He is risen." --bg=navy --textcolor=gold
```

| Option | Default | Description |
|---|---|---|
| `--bg=COLOR` | `black` | Background color |
| `--textcolor=COLOR` | `white` | Text color |

### Photo background

Use any JPEG or PNG as the background. The photo is resized and cropped to fill the canvas. A darkening pass (`--dim`) improves text readability over busy photos.

```bash
./textimage "He is risen." --bgphoto="sunset.jpg"
./textimage "He is risen." --bgphoto="photo.jpg" --dim=40
./textimage "He is risen." --bgphoto="photo.png" --dim=0 --textcolor=white
```

| Option | Default | Description |
|---|---|---|
| `--bgphoto=FILE` | _(none)_ | Background photo; overrides `--bg` |
| `--dim=N` | `50` | Darken photo 0–100% (0 = no dim, 100 = black) |

## Text Readability

### Panel

A semi-transparent colored rectangle drawn behind the text block improves contrast against photo backgrounds and on projector screens.

```bash
./textimage "He is risen." --bgphoto=photo.jpg --textpanel=60
./textimage "He is risen." --bgphoto=photo.jpg --textpanel=70 --textpanelcolor="#001020"
./textimage "He is risen." --textpanel=50 --textpanelrounded
```

| Option | Default | Description |
|---|---|---|
| `--textpanel=N` | `0` (off) | Panel opacity 1–100 |
| `--textpanelcolor=COLOR` | `black` | Panel color; any ImageMagick color |
| `--textpanelrounded` | off | Rounded corners on the panel |
| `--no-textpanelrounded` | _(default)_ | Square corners |

### Shadow

A drop shadow behind the text adds depth and separation from the background.

```bash
./textimage "He is risen." --bgphoto=photo.jpg --textshadow       # intensity 5 (default)
./textimage "He is risen." --textshadow=3                         # lighter shadow
./textimage "He is risen." --textshadow=8                         # heavier shadow
./textimage "He is risen." --no-textshadow                        # explicitly off
```

The `N` value ranges from 1 (lightest) to 10 (heaviest). The bare `--textshadow` flag uses intensity 5.

Panel and shadow can be combined:

```bash
./textimage "He is risen." --bgphoto=photo.jpg --textpanel=50 --textshadow
```

### Shadow Method

Control the rendering style with `--shadowmethod`:

| Value | Style | Description |
|---|---|---|
| `1` (default) | Soft / Gaussian | Black shadow blurred with a Gaussian filter at 80% opacity |
| `2` | Hard / copy | Solid black offset copy of the text, no blur |

```bash
./textimage "He is risen." --textshadow=6 --shadowmethod=1   # soft (default)
./textimage "He is risen." --textshadow=6 --shadowmethod=2   # hard
```

### Text Outline

Add a solid outline around text characters for stronger emphasis than a drop shadow. The outline is rendered cleanly outside the glyphs using morphological dilation, so the fill color is never obscured.

```bash
./textimage "He is risen." --textoutline            # 2px black outline (default width/color)
./textimage "He is risen." --textoutline=3          # 3px outline
./textimage "He is risen." --textoutline=4 --textoutlinecolor=navy
./textimage "He is risen." --no-textoutline         # off (default)
```

| Option | Description |
|---|---|
| `--textoutline[=N]` | Outline width in pixels (default 2 when flag used without value) |
| `--no-textoutline` | Disable outline (default) |
| `--textoutlinecolor=C` | Outline color, any ImageMagick color (default: black) |

When combined with `--textshadow`, the outline is applied first so the shadow is cast from the outlined shape, which looks natural. Outline width of 2–4px suits most large text; higher values produce bolder borders.

### Line Spacing

Adjust the amount of space between lines of text with `--linespacing`:

```bash
./textimage "He is risen." --linespacing=10    # more space between lines
./textimage "He is risen." --linespacing=-5    # tighter lines
./textimage "He is risen." --linespacing=0     # ImageMagick default (no adjustment)
```

Positive values increase the gap; negative values reduce it. The default is `0` (no adjustment).

## Position Offset

Fine-tune the vertical position of the text block with `--textoffy`. Positive moves it down, negative moves it up.

```bash
./textimage "He is risen." --textoffy=30    # move text 30px down
./textimage "He is risen." --textoffy=-20   # move text 20px up
```

Defaults to `0` (no adjustment).

## Text Size

By default the text auto-fits to the largest size that fills the canvas. Use one of these options to control the size — `--textsize`/`--maxtextsize` and `--textscale` cannot be combined.

**`--textscale=PCT`** — shrink the text area to a percentage of its default size. The font still auto-fits within the smaller area, so the result is proportionally smaller text:

```bash
./textimage "He is risen." --textscale=75    # ~75% of normal size
./textimage "He is risen." --textscale=50    # ~half size
```

**`--maxtextsize=N`** — cap the auto-fit font at N points. The font fills the canvas up to that maximum and won't exceed it:

```bash
./textimage "He is risen." --maxtextsize=140
./textimage "He is risen." --maxtextsize=80
```

**`--textsize=N`** — force the font to exactly N points, bypassing auto-fit entirely. Very long text may overflow the canvas at large sizes:

```bash
./textimage "He is risen." --textsize=160
./textimage "He is risen." --textsize=72
```

## Font

The default font is `Palatino` (macOS), `Palatino Linotype` (Windows), or `DejaVu-Serif` (Linux). Override with a font name or file path:

```bash
./textimage "He is risen." --font="Georgia"
./textimage "He is risen." --font="/Library/Fonts/MyFont.ttf"
```

To list fonts available to ImageMagick:
```bash
magick -list font
```

## Config File (.luminaverse [textimage])

Run `--saveconfig` to save the current settings as defaults for the current folder. Any subsequent `textimage` run in that folder will use those values automatically. Command-line arguments always override the config.

Save current settings as defaults:
```bash
./textimage --bg="#1a1a2e" --textcolor="#e8e8f0" --textscale=80 --saveconfig
```

Show current effective settings:
```bash
./textimage --showconfig
```

The `[textimage]` section of `.luminaverse` is plain text and can be edited by hand:
```
[textimage]
width            = 1920
height           = 1080
font             = /System/Library/Fonts/Palatino.ttc
bg               = #1a1a2e
bgphoto          =
dim              = 50
textcolor        = #e8e8f0
textsize         = 0
maxtextsize      = 0
textscale        = 100
textpanel        = 0
textpanelcolor   = black
textpanelrounded = no
textshadow       = no
shadowmethod     = 1
textoutline      = 0
textoutlinecolor = black
linespacing      = 0
textoffy         = 0
```

Supported keys: `width`, `height`, `font`, `bg`, `bgphoto`, `dim`, `textcolor`, `textsize`, `maxtextsize`, `textscale`, `textpanel`, `textpanelcolor`, `textpanelrounded`, `textshadow`, `shadowmethod`, `textoutline`, `textoutlinecolor`, `linespacing`, `textoffy`

## Files

- `textimage.cpp` — Source code
- `.luminaverse` — Shared config file; `[textimage]` section created by `--saveconfig`
