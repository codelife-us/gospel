// MIT License
//
// Copyright (c) 2026 Code Life
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

////////////////////////////////////////////////////////////////////////////////
// bvi.cpp  (Bible Verse Image)
// Renders a Bible verse reference to a JPEG image with auto-fitted text.
// Requires ImageMagick (convert).

#include <iostream>
#include <string>
#include <algorithm>
#include <map>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#define popen  _popen
#define pclose _pclose
#endif

using namespace std;

#ifdef _WIN32
#define HOME_ENV "USERPROFILE"
#else
#define HOME_ENV "HOME"
#endif

const string BVI_VERSION = "1.3";
const string CONFIG_FILE = ".bvi";

// ── Config file (.bvi in current directory) ───────────────────────────────────

// Read key=value pairs from CONFIG_FILE. Lines starting with # are comments.
map<string, string> loadConfig() {
    map<string, string> cfg;
    ifstream f(CONFIG_FILE);
    if (!f.good()) return cfg;
    string line;
    while (getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        // trim leading whitespace
        size_t s = line.find_first_not_of(" \t");
        if (s == string::npos || line[s] == '#') continue;
        line = line.substr(s);
        size_t eq = line.find('=');
        if (eq == string::npos) continue;
        string key = line.substr(0, eq);
        string val = line.substr(eq + 1);
        // trim trailing whitespace from key
        size_t ke = key.find_last_not_of(" \t");
        if (ke != string::npos) key = key.substr(0, ke + 1);
        // trim leading whitespace from value
        size_t vs = val.find_first_not_of(" \t");
        val = (vs != string::npos) ? val.substr(vs) : "";
        if (!key.empty()) cfg[key] = val;
    }
    return cfg;
}

// Return cfg[key] if present, otherwise defaultVal.
string cfgGet(const map<string, string>& cfg, const string& key, const string& defaultVal) {
    auto it = cfg.find(key);
    return (it != cfg.end()) ? it->second : defaultVal;
}

// ── Bible loading (same logic as gospel.cpp) ─────────────────────────────────

map<string, string> bibleVerses;

map<string, string> loadBible(const string& filename) {
    map<string, string> verses;
    ifstream file(filename);
    string line;
    while (getline(file, line)) {
        // Strip UTF-8 BOM if present
        if (line.size() >= 3 &&
            (unsigned char)line[0] == 0xEF &&
            (unsigned char)line[1] == 0xBB &&
            (unsigned char)line[2] == 0xBF)
            line = line.substr(3);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        size_t tab = line.find('\t');
        if (tab == string::npos) continue;
        string ref = line.substr(0, tab);
        if (ref.find(':') == string::npos) continue;
        verses[ref] = line.substr(tab + 1);
    }
    return verses;
}

// Looks up a single verse, range (8:9-10), or verse-to-end (8:20-).
string lookupVerses(const string& reference) {
    size_t colon = reference.rfind(':');
    if (colon != string::npos) {
        size_t dash = reference.find('-', colon);
        if (dash != string::npos) {
            string bookChapter = reference.substr(0, colon + 1);
            int startVerse = stoi(reference.substr(colon + 1, dash - colon - 1));
            int endVerse;
            string afterDash = reference.substr(dash + 1);
            if (afterDash.empty()) {
                endVerse = 0;
                for (const auto& entry : bibleVerses) {
                    if (entry.first.compare(0, bookChapter.size(), bookChapter) == 0) {
                        int v = stoi(entry.first.substr(bookChapter.size()));
                        if (v > endVerse) endVerse = v;
                    }
                }
                if (endVerse == 0) {
                    cerr << "Warning: no verses found for '" << reference << "'.\n";
                    return "";
                }
            } else {
                endVerse = stoi(afterDash);
            }
            string combined;
            for (int v = startVerse; v <= endVerse; ++v) {
                string key = bookChapter + to_string(v);
                auto it = bibleVerses.find(key);
                if (it != bibleVerses.end()) {
                    if (!combined.empty()) combined += " ";
                    combined += it->second;
                } else {
                    cerr << "Warning: verse not found: " << key << "\n";
                }
            }
            return combined;
        }
    }
    // Single verse
    auto it = bibleVerses.find(reference);
    if (it != bibleVerses.end()) return it->second;
    return "";
}

// ── Helpers ──────────────────────────────────────────────────────────────────

// Build a safe default filename from a reference like "Philippians 4:6-7"
string refToFilename(const string& ref) {
    string s;
    for (char c : ref) {
        if (c == ' ' || c == ':') s += '_';
        else s += c;
    }
    return s + ".jpg";
}

// Wraps s in single quotes, escaping any embedded single quotes.
string shellQuote(const string& s) {
    string r = "'";
    for (char c : s) {
        if (c == '\'') r += "'\\''";
        else r += c;
    }
    return r + "'";
}

// Returns "magick" if available, "convert" if not, or "" if neither found.
string detectIM() {
#ifdef _WIN32
    if (system("magick -version >NUL 2>&1") == 0) return "magick";
    if (system("convert -version >NUL 2>&1") == 0) return "convert";
#else
    if (system("magick -version >/dev/null 2>&1") == 0) return "magick";
    if (system("convert -version >/dev/null 2>&1") == 0) return "convert";
#endif
    return "";
}

void printHelp() {
    cout << "bvi v" << BVI_VERSION << "\n\n";
    cout << "Usage: bvi \"Reference\" [OPTIONS]\n\n";
    cout << "  Reference formats: \"Book Ch:V\"  \"Book Ch:V-V\"  \"Book Ch:V-\"\n\n";
    cout << "Options:\n";
    cout << "  -h, --help              Show this help message and exit\n";
    cout << "  -bv=VERSION             Bible version (KJV, BSB, WEB; default: KJV)\n";
    cout << "  --bibleversion=VERSION  Specify Bible version (KJV, BSB, WEB)\n";
    cout << "  --output=FILE           Output image file (default: <Reference>.jpg)\n";
    cout << "  --width=N               Image width in pixels (default: 1920)\n";
    cout << "  --height=N              Image height in pixels (default: 1080)\n";
    cout << "  --font=FONT             Font name or file path\n";
    cout << "  --bg=COLOR              Background color (default: black)\n";
    cout << "  --bgphoto=FILE          Background photo (jpg/png); overrides --bg\n";
    cout << "  --dim=N                 Darken photo 0-100% (default: 50); ignored without --bgphoto\n";
    cout << "  --textcolor=COLOR       Verse text color (default: white)\n";
    cout << "  --citecolor=COLOR       Citation text color (default: gray60)\n";
    cout << "  --quotes                Wrap verse text in \xe2\x80\x9c\xe2\x80\x9d quotation marks\n";
    cout << "  --no-quotes             Remove quotation marks (default)\n";
    cout << "  --citesize=N            Citation font size in points (default: auto ~30pt at 1080p)\n";
    cout << "  --citescale=PCT         Scale citation size to PCT% of auto (e.g. 75); cannot combine with --citesize\n";
    cout << "  --citestyle=STYLE       dash (default): \xe2\x80\x94 Ref (Ver)  |  parens: (Ref, Ver)  |  plain: Ref (Ver)\n";
    cout << "  --citeplacement=WHERE   bottom (default): near bottom edge  |  below: just under verse text\n";
    cout << "  --citebibleversion=VAL  yes (default): include Bible version in citation  |  no/false: omit it\n";
    cout << "  --textsize=N            Cap verse font at N points (absolute; cannot combine with --textscale)\n";
    cout << "  --textscale=PCT         Scale verse text area to PCT% of default (e.g. 75); cannot combine with --textsize\n\n";
    cout << "Config file (.bvi in current directory):\n";
    cout << "  --saveconfig            Save current settings to .bvi as new defaults\n";
    cout << "  --showconfig            Print current effective settings and exit\n\n";
    cout << "  Supported keys in .bvi:  bv  width  height  font  bg  bgphoto  dim  textcolor  citecolor  quotes  citesize  citescale  citestyle  citeplacement  citebibleversion  textsize  textscale\n\n";
    cout << "Requires:\n";
    cout << "  ImageMagick  —  brew install imagemagick\n\n";
    cout << "Examples:\n";
    cout << "  bvi \"Philippians 4:6-7\"\n";
    cout << "  bvi \"John 3:16\" -bv=BSB --output=john316.jpg\n";
    cout << "  bvi \"Romans 8:28\" --bg=\"#1a1a2e\" --textcolor=\"#e0e0e0\" --citecolor=\"#8888aa\"\n";
    cout << "  bvi --bg=navy --textcolor=gold --citecolor=lightyellow --saveconfig\n";
    cout << "\nTo list available fonts: magick -list font\n";
    cout << "  Or pass a font file path: --font=\"/path/to/font.ttf\"\n";
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    string im = detectIM();

    // ── Load config file first; command-line args override these values ───
    map<string, string> cfg = loadConfig();

#ifdef __APPLE__
    const string defaultFont = "/System/Library/Fonts/Palatino.ttc";
#elif defined(_WIN32)
    const string defaultFont = "C:/Windows/Fonts/pala.ttf";
#else
    const string defaultFont = "DejaVu-Serif";
#endif

    string version    = cfgGet(cfg, "bv",        "KJV");
    string reference;
    string outputFile;
    int imgWidth      = stoi(cfgGet(cfg, "width",     "1920"));
    int imgHeight     = stoi(cfgGet(cfg, "height",    "1080"));
    string font       = cfgGet(cfg, "font",           defaultFont);
    string bgColor    = cfgGet(cfg, "bg",             "black");
    string bgPhoto    = cfgGet(cfg, "bgphoto",        "");
    int    dimPct     = stoi(cfgGet(cfg, "dim",       "50"));
    string textColor  = cfgGet(cfg, "textcolor",      "white");
    string citeColor  = cfgGet(cfg, "citecolor",      "gray60");
    bool quotes       = cfgGet(cfg, "quotes",         "no") == "yes";
    int citeSizeOvr      = stoi(cfgGet(cfg, "citesize",      "0"));      // 0 = auto
    int citeScalePct     = stoi(cfgGet(cfg, "citescale",     "100"));    // 100 = auto base
    string citeStyle     = cfgGet(cfg, "citestyle",     "dash");         // dash | parens | plain
    string citePlacement = cfgGet(cfg, "citeplacement", "bottom");       // bottom | below
    bool citeBibleVersion = (cfgGet(cfg, "citebibleversion", "yes") != "no" &&
                             cfgGet(cfg, "citebibleversion", "yes") != "false");
    int textSizeOvr      = stoi(cfgGet(cfg, "textsize",      "0"));      // 0 = off (no cap)
    int textScalePct     = stoi(cfgGet(cfg, "textscale",     "100"));    // 100 = fill canvas

    bool saveConfig  = false;
    bool showConfig  = false;

    // ── Argument parsing ──────────────────────────────────────────────────
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printHelp();
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            cout << "bvi v" << BVI_VERSION << "\n";
            return 0;
        } else if (arg.find("-bv=") == 0) {
            version = arg.substr(4);
        } else if (arg.find("--bibleversion=") == 0) {
            version = arg.substr(arg.find('=') + 1);
        } else if (arg.find("--output=") == 0) {
            outputFile = arg.substr(9);
        } else if (arg.find("--width=") == 0) {
            imgWidth = stoi(arg.substr(8));
        } else if (arg.find("--height=") == 0) {
            imgHeight = stoi(arg.substr(9));
        } else if (arg.find("--font=") == 0) {
            font = arg.substr(7);
        } else if (arg.find("--bg=") == 0) {
            bgColor = arg.substr(5);
        } else if (arg.find("--bgphoto=") == 0) {
            bgPhoto = arg.substr(10);
        } else if (arg.find("--dim=") == 0) {
            dimPct = stoi(arg.substr(6));
        } else if (arg.find("--textcolor=") == 0) {
            textColor = arg.substr(12);
        } else if (arg.find("--citecolor=") == 0) {
            citeColor = arg.substr(12);
        } else if (arg == "--quotes") {
            quotes = true;
        } else if (arg == "--no-quotes") {
            quotes = false;
        } else if (arg.find("--citesize=") == 0) {
            citeSizeOvr = stoi(arg.substr(11));
        } else if (arg.find("--citescale=") == 0) {
            citeScalePct = stoi(arg.substr(12));
        } else if (arg.find("--citestyle=") == 0) {
            citeStyle = arg.substr(12);
        } else if (arg.find("--citeplacement=") == 0) {
            citePlacement = arg.substr(16);
        } else if (arg.find("--citebibleversion=") == 0) {
            string val = arg.substr(19);
            citeBibleVersion = (val != "no" && val != "false");
        } else if (arg.find("--textsize=") == 0) {
            textSizeOvr = stoi(arg.substr(11));
        } else if (arg.find("--textscale=") == 0) {
            textScalePct = stoi(arg.substr(12));
        } else if (arg == "--saveconfig") {
            saveConfig = true;
        } else if (arg == "--showconfig") {
            showConfig = true;
        } else if (arg.find("-") == 0) {
            cerr << "Error: unknown option '" << arg << "'\n";
            cerr << "Run 'bvi --help' for usage.\n";
            return 1;
        } else if (reference.empty()) {
            reference = arg;
        } else {
            cerr << "Error: unexpected argument '" << arg << "'\n";
            cerr << "Run 'bvi --help' for usage.\n";
            return 1;
        }
    }

    if (textSizeOvr > 0 && textScalePct != 100) {
        cerr << "Error: --textsize and --textscale cannot be used together.\n";
        return 1;
    }
    if (citeSizeOvr > 0 && citeScalePct != 100) {
        cerr << "Error: --citesize and --citescale cannot be used together.\n";
        return 1;
    }
    if (citeStyle != "dash" && citeStyle != "parens" && citeStyle != "plain") {
        cerr << "Error: --citestyle must be dash, parens, or plain.\n";
        return 1;
    }
    if (citePlacement != "bottom" && citePlacement != "below") {
        cerr << "Error: --citeplacement must be bottom or below.\n";
        return 1;
    }

    // ── --showconfig: print effective settings and exit ───────────────────
    if (showConfig) {
        cout << "Effective settings (config file + command-line):\n";
        cout << "  bv               = " << version   << "\n";
        cout << "  width            = " << imgWidth  << "\n";
        cout << "  height           = " << imgHeight << "\n";
        cout << "  font             = " << font      << "\n";
        cout << "  bg               = " << bgColor   << "\n";
        cout << "  bgphoto          = " << (bgPhoto.empty() ? "(none)" : bgPhoto) << "\n";
        cout << "  dim              = " << dimPct    << "\n";
        cout << "  textcolor        = " << textColor << "\n";
        cout << "  citecolor        = " << citeColor << "\n";
        cout << "  quotes           = " << (quotes ? "yes" : "no") << "\n";
        cout << "  citesize         = " << (citeSizeOvr > 0 ? to_string(citeSizeOvr) : "auto") << "\n";
        cout << "  citescale        = " << citeScalePct << "%\n";
        cout << "  citestyle        = " << citeStyle     << "\n";
        cout << "  citeplacement    = " << citePlacement << "\n";
        cout << "  citebibleversion = " << (citeBibleVersion ? "yes" : "no") << "\n";
        cout << "  textsize         = " << (textSizeOvr > 0 ? to_string(textSizeOvr) : "off") << "\n";
        cout << "  textscale        = " << textScalePct << "%\n";
        ifstream check(CONFIG_FILE);
        if (check.good())
            cout << "\nConfig file: ./" << CONFIG_FILE << " (loaded)\n";
        else
            cout << "\nConfig file: ./" << CONFIG_FILE << " (not found — using defaults)\n";
        return 0;
    }

    // ── --saveconfig: write current settings to .bvi and exit ─────────────
    if (saveConfig) {
        ofstream f(CONFIG_FILE);
        if (!f) {
            cerr << "Error: could not write '" << CONFIG_FILE << "'.\n";
            return 1;
        }
        f << "# bvi configuration — generated by bvi --saveconfig\n";
        f << "bv               = " << version   << "\n";
        f << "width            = " << imgWidth  << "\n";
        f << "height           = " << imgHeight << "\n";
        f << "font             = " << font      << "\n";
        f << "bg               = " << bgColor   << "\n";
        f << "bgphoto          = " << bgPhoto   << "\n";
        f << "dim              = " << dimPct    << "\n";
        f << "textcolor        = " << textColor << "\n";
        f << "citecolor        = " << citeColor << "\n";
        f << "quotes           = " << (quotes ? "yes" : "no") << "\n";
        f << "citesize         = " << citeSizeOvr   << "\n";
        f << "citescale        = " << citeScalePct  << "\n";
        f << "citestyle        = " << citeStyle     << "\n";
        f << "citeplacement    = " << citePlacement << "\n";
        f << "citebibleversion = " << (citeBibleVersion ? "yes" : "no") << "\n";
        f << "textsize         = " << textSizeOvr   << "\n";
        f << "textscale        = " << textScalePct  << "\n";
        cerr << "Saved defaults to ./" << CONFIG_FILE << "\n";
        return 0;
    }

    if (reference.empty()) {
        cerr << "Error: no Bible reference given.\n";
        cerr << "Usage: bvi \"Reference\" [OPTIONS]\n";
        cerr << "Example: bvi \"John 3:16\"\n";
        return 1;
    }

    if (outputFile.empty())
        outputFile = refToFilename(reference);

    // ── Bible version / file resolution ──────────────────────────────────
    transform(version.begin(), version.end(), version.begin(), ::toupper);

    string bibleFile, bibleUrl;
    if (version == "KJV") {
        bibleFile = "BibleKJV.txt";
        bibleUrl  = "https://openbible.com/textfiles/kjv.txt";
    } else if (version == "BSB") {
        bibleFile = "BibleBSB.txt";
        bibleUrl  = "https://bereanbible.com/bsb.txt";
    } else if (version == "WEB") {
        bibleFile = "BibleWEB.txt";
        bibleUrl  = "https://openbible.com/textfiles/web.txt";
    } else {
        cerr << "Error: unsupported Bible version '" << version << "'.\n";
        cerr << "Supported versions: KJV, BSB, WEB\n";
        return 1;
    }

    // Check Bible file; try HOME directory fallback, then offer to download
    {
        ifstream test(bibleFile);
        if (!test.good()) {
            const char* home = getenv(HOME_ENV);
            if (home) {
                string homePath = string(home) + "/" + bibleFile;
                ifstream homeTest(homePath);
                if (homeTest.good()) {
                    bibleFile = homePath;
                    goto bible_ready;
                }
            }
            cerr << "Bible file '" << bibleFile << "' not found.\n";
            cerr << "Download it now? (y/n): ";
            char answer;
            cin >> answer;
            if (answer == 'y' || answer == 'Y') {
                string cmd = "curl -L \"" + bibleUrl + "\" -o \"" + bibleFile + "\"";
                if (system(cmd.c_str()) != 0) {
                    cerr << "Download failed. Please download manually:\n  " << bibleUrl << "\n";
                    return 1;
                }
                cout << "Downloaded " << bibleFile << " successfully.\n";
            } else {
                cerr << "Cannot continue without Bible file. Exiting.\n";
                return 1;
            }
        }
    }
    bible_ready:

    bibleVerses = loadBible(bibleFile);

    // ── Verse lookup ──────────────────────────────────────────────────────
    string verseText = lookupVerses(reference);
    if (verseText.empty()) {
        cerr << "Error: reference not found: " << reference << "\n";
        return 1;
    }

    // Optionally wrap verse in curly quotation marks
    if (quotes)
        verseText = "\xe2\x80\x9c" + verseText + "\xe2\x80\x9d";

    // Citation line — format depends on --citestyle and --citebibleversion
    string citation;
    string ver = citeBibleVersion ? " (" + version + ")" : "";
    if (citeStyle == "parens")
        citation = citeBibleVersion ? "(" + reference + ", " + version + ")" : "(" + reference + ")";
    else if (citeStyle == "plain")
        citation = reference + ver;
    else // dash (default)
        citation = "\xe2\x80\x94 " + reference + ver;

    string quotedVerse    = shellQuote(verseText);
    string quotedCitation = shellQuote(citation);

    // ── Layout calculations ───────────────────────────────────────────────
    // Scale all measurements proportionally to the chosen image size.
    double scale = (double)imgHeight / 1080.0;

    // Verse text area: ~100px margin each side, and enough height to let
    // caption: auto-fit the font. The layer is trimmed after generation so
    // the actual text block (not the full canvas) is centered on the image.
    int verseW    = (int)(imgWidth  * 0.896 * textScalePct / 100.0);   // ~1720px at 1920 wide
    int verseH    = (int)(imgHeight * 0.741 * textScalePct / 100.0);  // ~800px at 1080 tall — caption auto-fits within this
    // Shift the trimmed verse block slightly above center so the citation fits below.
    int verseOffY = (int)(40 * scale);          // pixels above image center

    // Citation: fixed point size, placed near the bottom edge.
    int citePt    = (citeSizeOvr > 0) ? citeSizeOvr : max(1, (int)(30 * scale * citeScalePct / 100.0)); // ~30pt at 1080p
    int citeOffY  = max(20, (int)(55 * scale)); // pixels inward from bottom edge

    // Temp file for the intermediate verse layer PNG.
    string tmpLayer = outputFile + ".tmp_layer.png";

    // ── Step 1: Render verse text to a trimmed PNG layer ──────────────────
    //
    // caption:@file auto-sizes font to fill the given canvas.
    // -trim removes excess background so the layer height matches the text,
    // allowing gravity Center to truly center the text block on the canvas.
    //
    // Note: -size must be set here, not inherited from a prior xc: command,
    // to avoid the global -size leaking into the caption: dimensions.
    int borderH = max(10, (int)(20 * scale));
    int borderW = max(20, (int)(40 * scale));

    if (im.empty()) {
        cerr << "Error: ImageMagick not found. Install it with:\n";
        cerr << "  macOS:   brew install imagemagick\n";
        cerr << "  Linux:   apt install imagemagick\n";
        cerr << "  Windows: choco install imagemagick\n";
        return 1;
    }

    // When using a photo background the verse layer needs a transparent
    // background so it composites cleanly over the photo.
    string layerBg = bgPhoto.empty() ? bgColor : "none";

    ostringstream cmd1;
    cmd1 << im
         << " -background \"" << layerBg << "\""
         << " -fill \""       << textColor << "\""
         << " -font \""       << font << "\""
         << " -gravity Center"                        // center-align each text line
         << " -size "         << verseW << "x" << verseH
         << (textSizeOvr > 0 ? " -pointsize " + to_string(textSizeOvr) : "")
         << " caption:"      << quotedVerse
         << " -trim"                                  // crop to actual text bounds
         << " -bordercolor \"" << layerBg << "\""     // restore padding around text
         << " -border " << borderW << "x" << borderH
         << " \"" << tmpLayer << "\"";

    int ret1 = system(cmd1.str().c_str());
    if (ret1 != 0) {
        cerr << "Error: verse layer generation failed.\n\n";
        cerr << "To list fonts or use a font by path:\n";
        cerr << "  magick -list font\n";
        cerr << "  bvi \"...\" --font=\"/path/to/font.ttf\"\n";
        return 1;
    }

    // ── Citation annotation fragment ──────────────────────────────────────
    //
    // For "below" placement, query the trimmed layer height so the citation
    // can be positioned just underneath the verse text block.
    ostringstream citeAnnot;
    citeAnnot << " -fill \""    << citeColor << "\""
              << " -font \""    << font      << "\""
              << " -pointsize " << citePt;

    if (citePlacement == "below") {
        int layerH = 0;
        string identCmd = im + " identify -format \"%h\" \"" + tmpLayer + "\"";
        FILE* pipe = popen(identCmd.c_str(), "r");
        if (pipe) {
            char buf[32] = {};
            fgets(buf, sizeof(buf), pipe);
            pclose(pipe);
            layerH = atoi(buf);
        }
        int citeGap = max(5, (int)(12 * scale));
        int offset  = -verseOffY + layerH / 2 + citeGap;
        citeAnnot << " -gravity Center"
                  << (offset >= 0 ? " -annotate +0+" : " -annotate +0-")
                  << abs(offset);
    } else {
        citeAnnot << " -gravity South"
                  << " -annotate +0+" << citeOffY;
    }
    citeAnnot << " " << quotedCitation;

    // ── Step 2: Composite verse layer + annotate citation ─────────────────
    //
    // The trimmed verse layer is centered on the canvas (shifted slightly
    // above center to leave room for the citation at the bottom, or placed
    // inline when --citeplacement=below).
    ostringstream cmd2;
    if (bgPhoto.empty()) {
        cmd2 << im
             << " -size " << imgWidth << "x" << imgHeight
             << " xc:\"" << bgColor << "\""
             << " \"" << tmpLayer << "\""
             << " -gravity Center"
             << " -geometry +0-" << verseOffY
             << " -composite"
             << citeAnnot.str()
             << " \"" << outputFile << "\"";
    } else {
        // Load photo, resize to fill canvas, crop to exact size, then dim.
        int clampedDim = max(0, min(100, dimPct));
        cmd2 << im
             << " \"" << bgPhoto << "\""
             << " -resize " << imgWidth << "x" << imgHeight << "^"
             << " -gravity Center"
             << " -extent " << imgWidth << "x" << imgHeight
             << " -fill black -colorize " << clampedDim
             << " \"" << tmpLayer << "\""
             << " -gravity Center"
             << " -geometry +0-" << verseOffY
             << " -composite"
             << citeAnnot.str()
             << " \"" << outputFile << "\"";
    }

    int ret2 = system(cmd2.str().c_str());

    remove(tmpLayer.c_str());

    if (ret2 != 0) {
        cerr << "Error: image generation failed.\n\n";
        cerr << "To list fonts or use a font by path:\n";
        cerr << "  magick -list font\n";
        cerr << "  bvi \"...\" --font=\"/path/to/font.ttf\"\n";
        return 1;
    }

    cerr << "Saved to " << outputFile << "\n";
    return 0;
}
