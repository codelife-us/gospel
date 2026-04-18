# gospel

A C++ program that displays various gospel presentation tracts, biblical frameworks for sharing the Gospel of Jesus Christ. The program presents different gospel presentations with biblical explanations and references. The `--ref` option outputs Bible references directly without a tract.

## Features

- Support for multiple gospel presentation tracts/styles
- Currently includes four tracts: "The Romans Road" (default), "Somebody Loves You", "Have A Good Day", and "Are You Ready"
- Easy to add new tracts by extending the code
- Display tract content with biblical explanations
- Support for multiple Bible translations:
  - **KJV** (King James Version) - default
  - **BSB** (Berean Standard Bible)
  - **WEB** (World English Bible)
- Look up individual Bible verses, verse ranges, verse-to-end-of-chapter, or entire chapters directly with `--ref`
- Multiple references supported using comma separation
- Optional verse numbers prefixed to each verse with `--versenumbers` / `-vn`
- Each verse on its own line with `--versenewline` / `-vnl`
- Optional book and chapter header when outputting a full chapter with `--chapterheader` / `-ch`
- Configurable citation style: new line, inline, or parentheses (`--refstyle=1–4`)
- Optional curly quotes around verse text with `--versequotes`
- Italics for verse output opt-in with `--italic` (tract inline refs stay italic by default)
- Shorthand multi-reference: `"Psalm 7, 27"` is the same as `"Psalm 7, Psalm 27"`
- Save output to a file with `--output=`, including PDF via pandoc
- PDF font control: custom font (`--pdffont=`), size as percentage (`--pdffontsize=`), and margin (`--pdfmargin=`)
- Print PDF directly to printer with `--print`
- Batch export every tract in every Bible version as `.txt`, `.md`, and `.pdf` with `--outputall`
- Command-line configurable tract name and Bible version
- Invalid command line options are reported with an error
- Auto-prompts to download the Bible translation file if not found
- Persistent config file (`.gospel`) to save default settings with `--saveconfig`
- View effective settings (config + command-line) with `--showconfig`

## Building

**macOS / Linux:**
```bash
g++ -std=c++11 -o gospel gospel.cpp
```

**Windows (MSVC):**
```bat
cl /EHsc /std:c++17 /utf-8 gospel.cpp /Fe:gospel.exe
```

**Windows (MinGW / MSYS2):**
```bash
g++ -std=c++11 -o gospel.exe gospel.cpp
```

## Usage

Show help information:
```bash
./gospel -h
./gospel --help
```

Show version information:
```bash
./gospel -v
./gospel --version
```

Run with default KJV translation and default tract:
```bash
./gospel
```

Specify a different Bible version:
```bash
./gospel -bv=BSB
./gospel -bv=WEB
./gospel --bibleversion=BSB
```

Specify a different tract presentation:
```bash
./gospel -tn="The Romans Road"
./gospel --tractname="The Romans Road"
```

Combine tract and Bible version options:
```bash
./gospel -tn="The Romans Road" -bv=BSB
./gospel --tractname="The Romans Road" --bibleversion=WEB
```

Output as Markdown:
```bash
./gospel --outputtype=md > romansroad.md
```

Look up a single Bible reference directly (bypasses tract output):
```bash
./gospel --ref="John 3:16"
./gospel --ref="Romans 10:9-10" -bv=BSB
```

Look up multiple references using a comma to separate each:
```bash
./gospel --ref="John 3:16,Romans 5:8"
./gospel --ref="Romans 3:23,Romans 6:23,Romans 10:9-10"
```

Look up from a verse to the end of the chapter (omit the ending number after the dash):
```bash
./gospel --ref="Romans 8:20-"
./gospel --ref="John 3:16-" -bv=BSB
```

Look up an entire chapter:
```bash
./gospel --ref="Romans 8"
./gospel --ref="John 3" -bv=WEB
```

Look up multiple references using shorthand (book name carries forward):
```bash
./gospel --ref="Psalm 7, 27"          # same as "Psalm 7, Psalm 27"
./gospel --ref="1 John 4:9, 19, 5:1"  # same as "1 John 4:9, 1 John 19, 1 John 5:1"
```

Show verse numbers prefixed to each verse:
```bash
./gospel --ref="Romans 8" --versenumbers
./gospel --ref="John 3:16,Romans 5:8" -vn
```

Start each verse on its own line:
```bash
./gospel --ref="Romans 8" --versenewline
./gospel --ref="Romans 8" -vn -vnl
```

Print the book and chapter as a header when outputting a full chapter:
```bash
./gospel --ref="Romans 8" --chapterheader
./gospel --ref="Romans 8" -ch -vn -vnl
```

Wrap verse text in curly quotes:
```bash
./gospel --ref="John 3:16" --versequotes
```

Change the citation style (default is style 1):
```bash
# Style 1 (default): citation on its own line below the verse
./gospel --ref="John 3:16" --refstyle=1

# Style 2: citation inline, separated by a dash
./gospel --ref="John 3:16" --refstyle=2

# Style 3: reference in parentheses (no version)
./gospel --ref="John 3:16" --refstyle=3

# Style 4: reference and version in parentheses
./gospel --ref="John 3:16" --refstyle=4
```

Style 1 output example:
```
For God so loved the world...
— John 3:16 (KJV)
```

Style 2 output example:
```
For God so loved the world... - John 3:16 (KJV)
```

Style 3 output example:
```
For God so loved the world... (John 3:16)
```

Style 4 output example:
```
For God so loved the world... (John 3:16 (KJV))
```

Italicize verse output in `--ref` mode (off by default):
```bash
./gospel --ref="John 3:16" --italic
```

Save output to a file:
```bash
./gospel --output=romansroad.txt
./gospel --outputtype=md --output=romansroad.md
```

Save output as PDF (requires pandoc):
```bash
./gospel --output=romansroad.pdf
./gospel --ref="John 3:16,Romans 5:8" --output=verses.pdf
```

Print the PDF directly after generating:
```bash
./gospel --output=verse.pdf --print
./gospel --ref="John 3:16" --output=verse.pdf --print
```

Batch export all tracts in all Bible versions as `.txt`, `.md`, and `.pdf`:
```bash
./gospel --outputall
```

Files are saved in the current directory using the naming pattern `tractname_VERSION.ext` (tract name lowercased, spaces removed), for example:
```
theromansroad_KJV.txt
theromansroad_KJV.md
theromansroad_KJV.pdf
haveagoodday_BSB.txt
...
```

Bible versions are skipped silently if their translation file is not present. PDF generation requires pandoc and a LaTeX engine (same as `--output=.pdf`). PDF font and margin options (`--pdffont=`, `--pdfmargin=`, `--pdffontsize=`) apply to all generated PDFs.

To set or fix the default printer:

**macOS / Linux:**
```bash
lpoptions -d YourPrinterName   # set default printer
lpstat -p                      # list available printers
rm ~/.cups/lpoptions           # clear stale printer config if lpr errors occur
```

**Windows:** printing uses PowerShell `Start-Process -Verb Print` with your default PDF viewer.

Control the PDF margin (default is `0.5in`):
```bash
./gospel --output=romansroad.pdf --pdfmargin=0.75in
./gospel --output=romansroad.pdf --pdfmargin=2cm
```

Set a custom PDF font size as a percentage of the default (11pt base):
```bash
./gospel --output=romansroad.pdf --pdffontsize=120    # 120% → 13pt
./gospel --output=romansroad.pdf --pdffontsize=150    # 150% → 17pt
```

Color specific words or phrases in PDF output using HTML span tags in the tract data:
```cpp
"<span style=\"color: red;\">word</span>"        // named color
"<span style=\"color: #C0392B;\">word</span>"    // hex color
```
Colors are converted to LaTeX `\textcolor` automatically when generating a PDF.

Set a custom PDF font (requires xelatex):
```bash
./gospel --output=romansroad.pdf --pdffont="Georgia"
./gospel --output=romansroad.pdf --pdffont="Times New Roman"
```

Default fonts by platform: `Palatino` (macOS), `Palatino Linotype` (Windows), none/system default (Linux).

> **Linux note:** Palatino is not installed by default on Linux. The closest available equivalent included with texlive is `TeX Gyre Pagella`:
> ```bash
> ./gospel --output=romansroad.pdf --pdffont="TeX Gyre Pagella"
> ```

Install xelatex if needed:
```bash
# macOS / Linux
sudo tlmgr install xetex

# Windows
winget install MiKTeX.MiKTeX   # includes xelatex
```

If pandoc or a LaTeX engine is not installed the program will tell you how to install them and show the manual alternative:
```bash
./gospel --outputtype=md | pandoc -f markdown -o output.pdf
```

Install pandoc and a LaTeX engine:
```bash
# macOS
brew install pandoc && brew install --cask basictex
sudo tlmgr update --self

# Linux
apt install pandoc texlive

# Windows
winget install JohnMacFarlane.Pandoc
winget install MiKTeX.MiKTeX
```

> **macOS note:** After installing basictex, open a new Terminal window before running gospel. The `pdflatex` command is added to your PATH at login and will not be found in an existing Terminal session.

> **Windows note:** `curl` is built into Windows 10 and later and is used to download Bible translation files automatically. On Windows, `--print` uses PowerShell to send the PDF to your default printer.

## Configuration File

gospel reads defaults from a `.gospel` file in the current directory. Command-line arguments always override the config file.

Supported keys: `bv`, `tractname`, `refstyle`, `pdfmargin`, `pdffont`, `pdffontsize`, `outputtype`

Save current settings as new defaults:
```bash
./gospel -bv=BSB --refstyle=2 --saveconfig
```

This writes a `.gospel` file like:
```
# gospel configuration — generated by gospel --saveconfig
bv          = BSB
tractname   = The Romans Road
refstyle    = 2
pdfmargin   = 0.5in
pdffont     = Palatino
pdffontsize = 100
```

Print the effective settings (config file + any command-line overrides) and exit:
```bash
./gospel --showconfig
./gospel -bv=WEB --showconfig    # preview what would be used without running
```

To reset to built-in defaults, delete the config file:
```bash
rm .gospel
```

## Copying Output

The program output can be easily copied to the clipboard using piping:

**macOS:**
```bash
./gospel | pbcopy
```

**Linux:**
```bash
./gospel | xclip -selection clipboard
# or
./gospel | xsel -b
```

**Windows:**
```bat
gospel | clip
```

This is useful for sharing gospel presentations or copying the content for use in documents, emails, or other applications.

## Bible Translations

- **KJV**: King James Version — `BibleKJV.txt` (downloaded from https://openbible.com/textfiles/kjv.txt)
- **BSB**: Berean Standard Bible — `BibleBSB.txt` (downloaded from https://bereanbible.com/bsb.txt)
- **WEB**: World English Bible — `BibleWEB.txt` (downloaded from https://openbible.com/textfiles/web.txt)

If a Bible translation file is not found, the program will prompt you to download it automatically using `curl`.

## Files

- `gospel.cpp` - Main program source code
- `.gospel` - Optional config file for persistent defaults (created by `--saveconfig`)
- `BibleKJV.txt` - KJV Bible text (downloaded on first use)
- `BibleBSB.txt` - BSB Bible text (downloaded on first use)
- `BibleWEB.txt` - WEB Bible text (downloaded on first use)

## Requirements

- C++11 or later
- Standard C++ library
- `curl` (for automatic Bible file download — built into Windows 10+)
- `pandoc` + a LaTeX engine (for PDF output):
  - macOS: `basictex` via Homebrew
  - Linux: `texlive` via apt
  - Windows: `MiKTeX` via winget
- `xelatex` / `xetex` (for custom PDF fonts via `--pdffont=`)
