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
// bv.cpp
// Bible verse lookup tool -- outputs Bible references to stdout in plain text.

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <regex>
#include <fstream>
#include <sstream>
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#define popen  _popen
#define pclose _pclose
#else
#include <termios.h>
#include <unistd.h>
#endif

using namespace std;

#ifdef _WIN32
#define HOME_ENV "USERPROFILE"
#else
#define HOME_ENV "HOME"
#endif

const string VERSION = "1.0";
const string CONFIG_FILE = ".gospel";

map<string, string> loadConfig() {
    map<string, string> cfg;
    ifstream f(CONFIG_FILE);
    if (!f.good()) return cfg;
    string line;
    while (getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        size_t s = line.find_first_not_of(" \t");
        if (s == string::npos || line[s] == '#') continue;
        line = line.substr(s);
        size_t eq = line.find('=');
        if (eq == string::npos) continue;
        string key = line.substr(0, eq);
        string val = line.substr(eq + 1);
        size_t ke = key.find_last_not_of(" \t");
        if (ke != string::npos) key = key.substr(0, ke + 1);
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

// Function to load Bible verses from file
map<string, string> loadBible(const string& filename) {
    map<string, string> verses;
    ifstream file(filename);
    string line;
    while (getline(file, line)) {
        // Remove BOM if present
        if (line.size() >= 3 &&
            (unsigned char)line[0] == 0xEF &&
            (unsigned char)line[1] == 0xBB &&
            (unsigned char)line[2] == 0xBF) {
            line = line.substr(3);
        }
        // Remove carriage return if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        // Parse any line that looks like "Book Chapter:Verse\tText"
        size_t tab = line.find('\t');
        if (tab == string::npos) continue;
        string ref = line.substr(0, tab);
        // Ref must contain a colon to be a valid verse reference
        if (ref.find(':') == string::npos) continue;
        string text = line.substr(tab + 1);
        verses[ref] = text;
    }
    return verses;
}

// Bible verse database
map<string, string> bibleVerses;

// Look up one or more verses for a reference like "Romans 8:9" or "Romans 8:9-10"
// Returns the verse text, or empty string on failure. Writes errors to stderr.
string lookupVerses(const string& reference, bool verseNumbers = false, bool verseNewline = false, bool markdown = false) {
    // Check for a verse range (e.g. "Romans 8:9-10" or "Romans 8:20-")
    size_t colon = reference.rfind(':');
    if (colon != string::npos) {
        size_t dash = reference.find('-', colon);
        if (dash != string::npos) {
            string bookChapter = reference.substr(0, colon + 1); // "Romans 8:"
            int startVerse = stoi(reference.substr(colon + 1, dash - colon - 1));

            // Determine end verse: find last verse in chapter if nothing follows the dash
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
                    cerr << "Warning: no verses found for chapter in '" << reference << "'." << endl;
                    return "";
                }
            } else {
                endVerse = stoi(afterDash);
            }

            string combined;
            bool anyMissing = false;
            for (int v = startVerse; v <= endVerse; ++v) {
                string key = bookChapter + to_string(v);
                auto it = bibleVerses.find(key);
                if (it != bibleVerses.end()) {
                    if (!combined.empty()) combined += verseNewline ? (markdown ? "  \n" : "\n") : " ";
                    if (verseNumbers) combined += "[" + to_string(v) + "] ";
                    combined += it->second;
                } else {
                    cerr << "Warning: verse not found: " << key << endl;
                    anyMissing = true;
                }
            }
            if (combined.empty()) return "";
            if (anyMissing) {
                cerr << "Warning: some verses in range '" << reference << "' were not found." << endl;
            }
            return combined;
        }
    }
    // No colon: treat as entire chapter (e.g. "Romans 8")
    if (colon == string::npos) {
        string prefix = reference + ":";
        // Collect matching verses, keyed by verse number for correct ordering
        map<int, string> chapterVerses;
        for (const auto& entry : bibleVerses) {
            if (entry.first.compare(0, prefix.size(), prefix) == 0) {
                int verseNum = stoi(entry.first.substr(prefix.size()));
                chapterVerses[verseNum] = entry.second;
            }
        }
        if (chapterVerses.empty()) {
            cerr << "Warning: no verses found for chapter '" << reference << "'." << endl;
            return "";
        }
        string combined;
        for (const auto& v : chapterVerses) {
            if (!combined.empty()) combined += verseNewline ? (markdown ? "  \n" : "\n") : " ";
            if (verseNumbers) combined += "[" + to_string(v.first) + "] ";
            combined += v.second;
        }
        return combined;
    }

    // Single verse lookup — verse number comes from the reference itself
    auto it = bibleVerses.find(reference);
    if (it != bibleVerses.end()) {
        if (verseNumbers) {
            int verseNum = stoi(reference.substr(colon + 1));
            return "[" + to_string(verseNum) + "] " + it->second;
        }
        return it->second;
    }
    return "";
}

string formatCitation(const string& verseText, const string& reference, const string& version, bool markdown, int refStyle, bool italic = false, bool verseQuotes = false) {
    string citation = reference + " (" + version + ")";
    string quotedText = verseQuotes ? "\u201c" + verseText + "\u201d" : verseText;
    string formatted;
    if (refStyle == 2) {
        formatted = quotedText + " - " + citation;
    } else if (refStyle == 3) {
        formatted = quotedText + " (" + reference + ")";
    } else if (refStyle == 4) {
        formatted = quotedText + " (" + citation + ")";
    } else {
        formatted = quotedText + "  \n\u2014 " + citation;
    }
    if (markdown && italic) {
        formatted = "*" + formatted + "*";
    }
    return formatted;
}

void printHelp() {
    cout << "bv v" << VERSION << endl;
    cout << "\nUsage: bv --ref=REF [OPTIONS]" << endl;
    cout << "\nOptions:" << endl;
    cout << "  -h, --help              Show this help message and exit" << endl;
    cout << "  -v, --version           Show version information and exit" << endl;
    cout << "  -bv=VERSION             Set Bible version (default: KJV)" << endl;
    cout << "  --bibleversion=VERSION  Specify Bible version (KJV, BSB, WEB)" << endl;
    cout << "  --ref=REF               Bible reference (use comma to separate multiple)" << endl;
    cout << "                          Formats: Book Ch:V  Book Ch:V-V  Book Ch:V-  Book Ch" << endl;
    cout << "  --refstyle=STYLE        Citation style: 1=new line (default), 2=inline, 3=parens, 4=parens+version" << endl;
    cout << "  --versenumbers, -vn     Prefix each verse with its verse number, e.g. [1]" << endl;
    cout << "  --versenewline, -vnl    Start each verse on a new line" << endl;
    cout << "  --italic                Italicize verse text" << endl;
    cout << "  --versequotes           Wrap each verse in curly quotes" << endl;
    cout << "  --chapterheader, -ch    Print book and chapter as a header for full chapters" << endl;
    cout << "\nConfig file (.gospel in current directory):" << endl;
    cout << "  --saveconfig            Save current settings to .gospel as new defaults" << endl;
    cout << "  --showconfig            Print current effective settings and exit" << endl;
    cout << "  Supported keys:  bv  refstyle  versequotes" << endl;
    cout << "\nExamples:" << endl;
    cout << "  bv --ref=\"John 3:16\"                    Single verse" << endl;
    cout << "  bv --ref=\"John 3:16,Romans 5:8\"         Multiple verses" << endl;
    cout << "  bv --ref=\"Romans 8:20-\"                  Verse to end of chapter" << endl;
    cout << "  bv --ref=\"Romans 8\" -vn                  Full chapter with verse numbers" << endl;
    cout << "  bv --ref=\"Psalm 7, 27\"                   Shorthand: Psalm 7 and Psalm 27" << endl;
    cout << "  bv -bv=BSB --ref=\"John 3:16\"             Specific Bible version" << endl;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    map<string, string> cfg = loadConfig();

    string version  = cfgGet(cfg, "bv",          "KJV");
    int refStyle    = stoi(cfgGet(cfg, "refstyle", "1"));
    bool verseQuotes = cfgGet(cfg, "versequotes", "0") == "1";

    string refArg;
    bool verseNumbers  = false;
    bool verseNewline  = false;
    bool italic        = false;
    bool chapterHeader = false;
    bool saveConfig    = false;
    bool showConfig    = false;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printHelp(); return 0;
        } else if (arg == "-v" || arg == "--version") {
            cout << "bv v" << VERSION << endl; return 0;
        } else if (arg.find("-bv=") == 0) {
            version = arg.substr(arg.find('=') + 1);
        } else if (arg.find("--bibleversion=") == 0) {
            version = arg.substr(arg.find('=') + 1);
        } else if (arg.find("--ref=") == 0) {
            refArg = arg.substr(arg.find('=') + 1);
        } else if (arg.find("--refstyle=") == 0) {
            refStyle = stoi(arg.substr(arg.find('=') + 1));
        } else if (arg == "--versenumbers" || arg == "-vn") {
            verseNumbers = true;
        } else if (arg == "--versenewline" || arg == "-vnl") {
            verseNewline = true;
        } else if (arg == "--italic") {
            italic = true;
        } else if (arg == "--versequotes") {
            verseQuotes = true;
        } else if (arg == "--chapterheader" || arg == "-ch") {
            chapterHeader = true;
        } else if (arg == "--saveconfig") {
            saveConfig = true;
        } else if (arg == "--showconfig") {
            showConfig = true;
        } else if (arg.find("-") == 0) {
            cerr << "Error: unknown option '" << arg << "'" << endl;
            cerr << "Run 'bv --help' for usage." << endl;
            return 1;
        }
    }

    if (showConfig) {
        cout << "Effective settings:" << endl;
        cout << "  bv          = " << version              << endl;
        cout << "  refstyle    = " << refStyle             << endl;
        cout << "  versequotes = " << (verseQuotes ? 1 : 0) << endl;
        ifstream check(CONFIG_FILE);
        cout << "\nConfig file: ./" << CONFIG_FILE
             << (check.good() ? " (loaded)" : " (not found -- using defaults)") << endl;
        return 0;
    }

    if (saveConfig) {
        ofstream f(CONFIG_FILE);
        if (!f) { cerr << "Error: could not write '" << CONFIG_FILE << "'." << endl; return 1; }
        f << "# bv configuration -- generated by bv --saveconfig\n";
        f << "bv          = " << version              << "\n";
        f << "refstyle    = " << refStyle             << "\n";
        f << "versequotes = " << (verseQuotes ? 1 : 0) << "\n";
        cerr << "Saved defaults to ./" << CONFIG_FILE << endl;
        return 0;
    }

    if (refArg.empty()) {
        cerr << "Usage: bv --ref=REF [OPTIONS]" << endl;
        cerr << "Run 'bv --help' for usage." << endl;
        return 1;
    }

    transform(version.begin(), version.end(), version.begin(), ::toupper);

    string bibleFile, bibleUrl;
    if (version == "KJV") {
        bibleFile = "BibleKJV.txt"; bibleUrl = "https://openbible.com/textfiles/kjv.txt";
    } else if (version == "BSB") {
        bibleFile = "BibleBSB.txt"; bibleUrl = "https://bereanbible.com/bsb.txt";
    } else if (version == "WEB") {
        bibleFile = "BibleWEB.txt"; bibleUrl = "https://openbible.com/textfiles/web.txt";
    } else {
        cerr << "Error: unsupported Bible version '" << version << "'." << endl;
        cerr << "Supported versions: KJV, BSB, WEB" << endl;
        return 1;
    }

    {
        ifstream test(bibleFile);
        if (!test.good()) {
            // Try $HOME directory
            const char* home = getenv(HOME_ENV);
            if (home) {
                string homePath = string(home) + "/" + bibleFile;
                ifstream homeTest(homePath);
                if (homeTest.good()) {
                    bibleFile = homePath;
                    goto bible_ready;
                }
            }
            cerr << "Bible file '" << bibleFile << "' not found." << endl;
            cerr << "Download it now? (y/n): ";
            char answer; cin >> answer;
            if (answer == 'y' || answer == 'Y') {
                string cmd = "curl -L \"" + bibleUrl + "\" -o \"" + bibleFile + "\"";
                if (system(cmd.c_str()) != 0) {
                    cerr << "Download failed. Please download manually:\n  " << bibleUrl << endl;
                    return 1;
                }
                cout << "Downloaded " << bibleFile << " successfully." << endl;
            } else {
                cerr << "Cannot continue without a Bible file. Exiting." << endl;
                return 1;
            }
        }
    }
    bible_ready:

    bibleVerses = loadBible(bibleFile);

    stringstream ss(refArg);
    string token, lastBook;
    while (getline(ss, token, ',')) {
        if (!token.empty() && token.front() == '[') token.erase(0, 1);
        if (!token.empty() && token.back()  == ']') token.pop_back();
        size_t start = token.find_first_not_of(" \t");
        size_t end   = token.find_last_not_of(" \t");
        if (start == string::npos) continue;
        token = token.substr(start, end - start + 1);
        if (!token.empty() && isdigit((unsigned char)token[0]) && !lastBook.empty()) {
            token = lastBook + " " + token;
        } else {
            size_t bookEnd = string::npos;
            for (size_t i = 0; i < token.size(); ++i) {
                if (isdigit((unsigned char)token[i])) {
                    size_t j = i;
                    while (j > 0 && token[j-1] == ' ') --j;
                    if (j > 0) bookEnd = j;
                    break;
                }
            }
            if (bookEnd != string::npos) lastBook = token.substr(0, bookEnd);
        }
        string verseText = lookupVerses(token, verseNumbers, verseNewline, false);
        if (!verseText.empty()) {
            if (chapterHeader && token.find(':') == string::npos)
                cout << token << "\n\n";
            cout << formatCitation(verseText, token, version, false, refStyle, italic, verseQuotes) << endl << endl;
        } else {
            cerr << "Reference not found: " << token << endl;
        }
    }

    return 0;
}
