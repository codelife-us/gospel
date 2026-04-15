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
// gospel.cpp
// This program outputs various gospel presentation tracts, biblical frameworks for sharing the Gospel of Jesus Christ.
// Also there is the --ref option to output only Bible references from one or many references provided.

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <regex>
#include <fstream>
#include <sstream>
#include <cstdio>

using namespace std;

const string VERSION = "1.1";

struct Section {
    string section_title;
    string text;
};

struct Tract {
    string name;
    vector<Section> sections;
};

// Romans Road sections
vector<Section> romansRoadSections = {
    {"What is The Romans Road?", "\nSimply put, the Romans Road shows the path to Heaven. It uses points from the book of Romans in the Bible to explain God’s gift of salvation. At each stop we learn something new about why we need salvation, how God has provided for it, and how we receive it.\n"
"\nAs you look at each of the six points outlined here, take time to reflect on what God’s Word has to say. Remember that the book of Romans was written to people just like you and me: people living in a busy culture, trying to make sense of what is true and what is most important in life.\n"
"\nThe Romans Road describes the only way to Heaven. Study it, learn it, and follow it!"},
    {"1. Everyone Needs Salvation", "[Romans 3:23]"},
    {"2. Jesus Died For Our Salvation", "[Romans 5:8]"},
    {"3. Salvation Is A Gift", "[Romans 6:23]"},
    {"4. We Are Saved By Grace", "[Romans 11:6]"},
    {"5. Salvation Comes Through Faith", "[Romans 4:5]"},
    {"6. God Saves All Who Call Upon Him", "[Romans 10:13]\n"},
    {"","Follow the Romans Road to salvation today. Recognize that you are a sinner and that your sin must be judged by God. See that Jesus died to pay the penalty for your sin, but that you must choose to accept His provision. Understand that you cannot earn your way to Heaven through good works or religious activity. Now turn to God and put your faith in Jesus Christ who died for you and rose again.\n"},
    {"","[Romans 10:9-10]"}
};

vector<Section> tracts2 = {
    {"<span style=\"color: red;\">Somebody Loves You</span>", "\nThe creator of all things loves <span style=\"color: #0c9c30;\">YOU</span> so much that He sent His only Son, Jesus, to die for you on the cross. He wants to forgive your sins and make you clean so that you can be with Him for all eternity. Isn’t it amazing to know that you are loved by God?\n\n**HIS LOVE FOR YOU IS:...**"},
    {"<span style=\"color: red;\">Unending</span>", "— God’s love is eternal. He loved you yesterday, loves you today and will love you forever!\n\n[Jeremiah 31:3]"},
    {"<span style=\"color: red;\">Unselfish</span>", "— God didn’t wait for us to love Him first. His love was freely given.\n\n[1 John 4:19]\n"},
    {"<span style=\"color: red;\">Undeserved</span>", "— God is holy, righteous, and just. As sinners, we have done nothing to deserve God’s love. But He loves us anyway.\n\n[Romans 5:8]\n"},
    {"<span style=\"color: red;\">Unimaginable</span>", "— God’s love led Him to send His son Jesus to bear the punishment for our sins on the cross. Can you imagine watching someone you love being punished to the point of death for something they didn’t do?\n\n[John 3:16]\n"},
    {"<span style=\"color: red;\">Undying</span>", "— God didn’t only send Jesus to die for our sins. He then raised Him from the dead so we can have eternal life. What a wonderful gift He offers us!\n\n[1 John 4:9]\n"},
    {"<span style=\"color: red;\">Unmerited</span>", "— God offers you the free gift of salvation through Jesus Christ. Because it is a gift, you cannot earn it. Instead, you must see your need of salvation and accept it by trusting Jesus as your Savior.\n\n[Ephesians 2:8-9]\n\nHow can you receive God’s gift of salvation? The Bible says:\n\n[Romans 10:9]\n\nIt’s that easy! Will you choose to accept God’s love? Then you too can experience God’s unfathomable love for all eternity!\n\n[Romans 8:38-39]\n"}
};

// Available tracts
map<string, Tract> availableTracts = {
    {"The Romans Road", {"The Romans Road", romansRoadSections}},
    {"Somebody Loves You", {"Somebody Loves You", tracts2}}
};

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
string lookupVerses(const string& reference, bool verseNumbers = false) {
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
                    if (!combined.empty()) combined += " ";
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
            if (!combined.empty()) combined += " ";
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

// Registry of hex colors -> auto-generated LaTeX color names.
// Populated during text processing; written as \definecolor into the PDF header.
map<string, string> pdfColorRegistry;
int pdfColorCounter = 0;

string registerPdfColor(const string& hex) {
    auto it = pdfColorRegistry.find(hex);
    if (it != pdfColorRegistry.end()) return it->second;
    string name = "gospelcolor" + to_string(pdfColorCounter++);
    pdfColorRegistry[hex] = name;
    return name;
}

// Wrap text in \textcolor{name}{text} for PDF output.
// Hex colors are registered for \definecolor in the header; named colors used directly.
string applyPdfColor(const string& text, const string& color) {
    if (color.empty()) return text;
    string c = color;
    if (!c.empty() && c[0] == '#') c = c.substr(1);
    bool isHex = c.size() == 6 && c.find_first_not_of("0123456789abcdefABCDEF") == string::npos;
    if (isHex) {
        transform(c.begin(), c.end(), c.begin(), ::toupper);
        return "\\textcolor{" + registerPdfColor(c) + "}{" + text + "}";
    }
    return "\\textcolor{" + color + "}{" + text + "}";
}

// Convert <span style="color: #hex;">text</span> to \textcolor[HTML]{HEX}{text} for PDF output.
// Supports hex colors (#RRGGBB) and named colors (red, blue, etc.).
string convertSpansToPdfColor(const string& text) {
    string result;
    regex spanPattern("<span\\s+style=[\"']color:\\s*([^\"';]+);?\\s*[\"']>(.*?)</span>",
                      regex::icase);
    sregex_iterator it(text.cbegin(), text.cend(), spanPattern);
    sregex_iterator end;
    size_t lastEnd = 0;

    for (; it != end; ++it) {
        const smatch& m = *it;
        result += text.substr(lastEnd, m.position() - lastEnd);
        string color = m[1].str();
        size_t s = color.find_first_not_of(" \t");
        size_t e = color.find_last_not_of(" \t");
        if (s != string::npos) color = color.substr(s, e - s + 1);
        result += applyPdfColor(m[2].str(), color);
        lastEnd = m.position() + m.length();
    }
    result += text.substr(lastEnd);
    return result;
}

// Process markdown text with embedded bible references
string processMarkdownReferences(const string& text, const string& version, bool markdown, int refStyle, bool verseNumbers, bool verseQuotes = false, bool isPdf = false) {
    string result = isPdf ? convertSpansToPdfColor(text) : text;
    regex refPattern("\\[([^\\]]+)\\]");
    smatch match;
    string::const_iterator searchStart(text.cbegin());
    size_t offset = 0;

    while (regex_search(searchStart, text.cend(), match, refPattern)) {
        string reference = match[1].str();

        string verseText = lookupVerses(reference, verseNumbers);
        if (!verseText.empty()) {
            string replacement = formatCitation(verseText, reference, version, markdown, refStyle, true, verseQuotes);

            // Replace the [Reference] with the verse text and formatted reference
            size_t pos = result.find("[" + reference + "]", offset);
            if (pos != string::npos) {
                result.replace(pos, reference.length() + 2, replacement);
                offset = pos + replacement.length();
            }
        }

        searchStart = match.suffix().first;
    }

    return result;
}

void printHelp() {
    cout << "gospel v" << VERSION << endl;
    cout << "\nUsage: gospel [OPTIONS]" << endl;
    cout << "\nOptions:" << endl;
    cout << "  -h, --help              Show this help message and exit" << endl;
    cout << "  -v, --version           Show version information and exit" << endl;
    cout << "  -bv=VERSION             Set Bible version (default: KJV)" << endl;
    cout << "  --bibleversion=VERSION  Specify Bible version (KJV, BSB, WEB)" << endl;
    cout << "  --outputtype=TYPE       Set output type (plaintext, md)" << endl;
    cout << "  -tn=NAME                Set tract name (default: 'The Romans Road')" << endl;
    cout << "  --tractname=NAME        Specify tract presentation by name" << endl;
    cout << "  --ref=REF                Output a Bible reference directly (use comma to separate multiple)" << endl;
    cout << "                           REF formats: Book Ch:V  Book Ch:V-V  Book Ch:V-  Book Ch" << endl;
    cout << "  --refstyle=STYLE         Citation style: 1=new line (default), 2=inline, 3=parentheses, 4=parentheses with version" << endl;
    cout << "  --versenumbers, -vn      Prefix each verse with its verse number, e.g. [1]" << endl;
    cout << "  --output=FILE            Write output to FILE (.pdf requires pandoc)" << endl;
    cout << "  --pdfmargin=MARGIN       PDF margin size (default: 0.5in, e.g. 0.75in, 2cm)" << endl;
    cout << "  --pdffont=FONT           PDF font name (default: Palatino, requires xelatex)" << endl;
    cout << "  --pdffontsize=PCT        PDF font size as a percentage (default: 100, e.g. 120 = 120%)" << endl;
    cout << "  --italic                 Italicize verse output (default: off for --ref, on for tracts)" << endl;
    cout << "  --versequotes            Wrap each Bible verse in curly quotes" << endl;
    cout << "  --print                  Send PDF to printer after generating (requires --output=.pdf)" << endl;
    cout << "\nExamples:" << endl;
    cout << "  gospel --outputtype=md                            Display tract output as markdown" << endl;
    cout << "  gospel                                            Display default tract in KJV" << endl;
    cout << "  gospel -bv=BSB                                    Display default tract in BSB" << endl;
    cout << "  gospel -tn=\"The Romans Road\" -bv=KJV              Display Romans Road in KJV" << endl;
    cout << "  gospel --ref=\"John 3:16\"                          Display a single verse" << endl;
    cout << "  gospel --ref=\"John 3:16,Romans 8:9-10\"           Display multiple verses" << endl;
    cout << "  gospel --ref=\"Romans 8:20-\"                       Display verse 20 to end of chapter" << endl;
    cout << "  gospel --ref=\"Romans 8\" -vn                       Display a full chapter with verse numbers" << endl;
    cout << "  gospel --output=tract.pdf                         Save tract as PDF (requires pandoc)" << endl;
    cout << "  gospel --ref=\"John 3:16\" --output=verse.pdf       Save verse as PDF (requires pandoc)" << endl;
}
int main(int argc, char* argv[]) {
    string version = "KJV";
    string tractName = "The Romans Road"; // Default tract
    string outputType = "plaintext";
    string refArg;
    string outputFile;
    string pdfMargin = "0.5in";
#ifdef __APPLE__
    string pdfFont = "Palatino";
#else
    string pdfFont;
#endif
    int pdfFontSizePct = 100;
    int refStyle = 1;
    bool verseNumbers = false;
    bool italic = false;
    bool printPdf = false;
    bool verseQuotes = false;

    // Parse command-line arguments
    for(int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printHelp();
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            cout << "gospel v" << VERSION << endl;
            return 0;
        } else if (arg.find("-bv=") == 0) {
            size_t eq = arg.find('=');
            if (eq != string::npos) {
                version = arg.substr(eq + 1);
            }
        } else if (arg.find("--bibleversion=") == 0) {
            size_t eq = arg.find('=');
            if (eq != string::npos) {
                version = arg.substr(eq + 1);
            }
        } else if (arg.find("-tn=") == 0) {
            size_t eq = arg.find('=');
            if (eq != string::npos) {
                tractName = arg.substr(eq + 1);
            }
        } else if (arg.find("--tractname=") == 0) {
            size_t eq = arg.find('=');
            if (eq != string::npos) {
                tractName = arg.substr(eq + 1);
            }
        } else if (arg.find("--outputtype=") == 0) {
            size_t eq = arg.find('=');
            if (eq != string::npos) {
                outputType = arg.substr(eq + 1);
            }
        } else if (arg.find("--ref=") == 0) {
            size_t eq = arg.find('=');
            if (eq != string::npos) {
                refArg = arg.substr(eq + 1);
            }
        } else if (arg.find("--refstyle=") == 0) {
            size_t eq = arg.find('=');
            if (eq != string::npos) {
                refStyle = stoi(arg.substr(eq + 1));
            }
        } else if (arg == "--versenumbers" || arg == "-vn") {
            verseNumbers = true;
        } else if (arg.find("--output=") == 0) {
            size_t eq = arg.find('=');
            if (eq != string::npos) {
                outputFile = arg.substr(eq + 1);
            }
        } else if (arg.find("--pdfmargin=") == 0 || arg.find("-pdfmargin=") == 0) {
            size_t eq = arg.find('=');
            if (eq != string::npos) {
                pdfMargin = arg.substr(eq + 1);
            }
        } else if (arg.find("--pdffont=") == 0 || arg.find("-pdffont=") == 0) {
            size_t eq = arg.find('=');
            if (eq != string::npos) {
                pdfFont = arg.substr(eq + 1);
            }
        } else if (arg.find("--pdffontsize=") == 0 || arg.find("-pdffontsize=") == 0) {
            size_t eq = arg.find('=');
            if (eq != string::npos) {
                pdfFontSizePct = stoi(arg.substr(eq + 1));
            }
        } else if (arg == "--italic") {
            italic = true;
        } else if (arg == "--versequotes") {
            verseQuotes = true;
        } else if (arg == "--print") {
            printPdf = true;
        } else if (arg.find("-") == 0) {
            cerr << "Error: unknown option '" << arg << "'" << endl;
            cerr << "Run 'gospel --help' for usage." << endl;
            return 1;
        }
    }
    
    // Normalize version to uppercase for comparison
    transform(version.begin(), version.end(), version.begin(), ::toupper);

    // Load Bible verses based on selected version
    string bibleFile;
    string bibleUrl;
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
        cerr << "Error: unsupported Bible version '" << version << "'." << endl;
        cerr << "Supported versions: KJV, BSB, WEB" << endl;
        return 1;
    }

    // Check that the Bible file exists; offer to download it if not
    {
        ifstream test(bibleFile);
        if (!test.good()) {
            cerr << "Bible file '" << bibleFile << "' not found." << endl;
            cerr << "Download it now? (y/n): ";
            char answer;
            cin >> answer;
            if (answer == 'y' || answer == 'Y') {
                string cmd = "curl -L \"" + bibleUrl + "\" -o \"" + bibleFile + "\"";
                int ret = system(cmd.c_str());
                if (ret != 0) {
                    cerr << "Download failed. Please download manually:" << endl;
                    cerr << "  " << bibleUrl << endl;
                    return 1;
                }
                cout << "Downloaded " << bibleFile << " successfully." << endl;
            } else {
                cerr << "Cannot continue without a Bible file. Exiting." << endl;
                return 1;
            }
        }
    }

    bibleVerses = loadBible(bibleFile);

    // PDF output always requires markdown as the intermediate format
    bool isPdf = outputFile.size() >= 4 &&
                 outputFile.substr(outputFile.size() - 4) == ".pdf";
    bool markdown = isPdf || (outputType == "md" || outputType == "MD");

    // Capture all output into a string so we can route it to stdout or a file
    ostringstream out;

    // --ref mode: output only the requested Bible references
    if (!refArg.empty()) {
        stringstream ss(refArg);
        string token;
        while (getline(ss, token, ',')) {
            // Trim optional [] wrappers
            if (!token.empty() && token.front() == '[') token.erase(0, 1);
            if (!token.empty() && token.back()  == ']') token.pop_back();
            // Trim whitespace
            size_t start = token.find_first_not_of(" \t");
            size_t end   = token.find_last_not_of(" \t");
            if (start == string::npos) continue;
            token = token.substr(start, end - start + 1);

            string verseText = lookupVerses(token, verseNumbers);
            if (!verseText.empty()) {
                out << formatCitation(verseText, token, version, markdown, refStyle, italic, verseQuotes) << endl << endl;
            } else {
                cerr << "Reference not found: " << token << endl;
            }
        }
    } else {
        // Check if tract exists
        if (availableTracts.find(tractName) == availableTracts.end()) {
            cerr << "Error: Tract '" << tractName << "' not found." << endl;
            cerr << "Available tracts:" << endl;
            for (const auto& tract : availableTracts) {
                cerr << "  - " << tract.first << endl;
            }
            return 1;
        }

        const Tract& selectedTract = availableTracts[tractName];

        for (const auto& v : selectedTract.sections) {
            if (v.section_title.length() > 0) {
                bool textOnSameLine = v.text.length() > 0 && v.text[0] != '\n';
                if (markdown) {
                    string title = isPdf ? convertSpansToPdfColor(v.section_title) : v.section_title;
                    if (textOnSameLine) {
                        out << "\n**" << title << "**";
                    } else {
                        out << "\n## " << title;
                    }
                } else {
                    out << v.section_title;
                }
                if (v.text.length() > 0) {
                    string processedText = processMarkdownReferences(v.text, version, markdown, refStyle, verseNumbers, verseQuotes, isPdf);
                    if (markdown) {
                        string hardBreak;
                        for (size_t i = 0; i < processedText.size(); ++i) {
                            if (processedText[i] == '\n' && (i < 2 || processedText[i-1] != ' ' || processedText[i-2] != ' ')) {
                                hardBreak += "  \n";
                            } else {
                                hardBreak += processedText[i];
                            }
                        }
                        processedText = hardBreak;
                    }
                    out << " " << processedText << "\n" << endl;
                } else {
                    out << "\n" << endl;
                }
            } else if (v.text.length() > 0) {
                string processedText = processMarkdownReferences(v.text, version, markdown, refStyle, verseNumbers, verseQuotes, isPdf);
                out << processedText << "\n" << endl;
            }
        }
    }

    // Route output
    if (outputFile.empty()) {
        cout << out.str();
    } else if (isPdf) {
        // Write markdown to a temp file then convert via pandoc
        string tmpFile = outputFile + ".tmp.md";
        ofstream tmp(tmpFile);
        if (!tmp) {
            cerr << "Error: could not create temporary file '" << tmpFile << "'." << endl;
            return 1;
        }
        tmp << out.str();
        tmp.close();

        // Build pandoc command
        string cmd = "pandoc -f markdown+raw_tex -V geometry:margin=" + pdfMargin;
        string headerFile = outputFile + ".tmp.tex";

        // Use xelatex + fontspec Scale to handle font and font size together.
        // Scale= is relative to the document base (11pt), so 150% = Scale=1.5.
        // This is more reliable than \fontsize with xelatex since fontspec
        // controls font rendering and overrides \fontsize in AtBeginDocument.
        double scale = pdfFontSizePct / 100.0;
        bool needHeader = !pdfFont.empty() || !pdfColorRegistry.empty();
        if (needHeader) {
            if (!pdfFont.empty()) {
                // Use xelatex and set the font + scale together in a header file.
                // We do NOT pass -V mainfont= to pandoc because the template calls
                // \setmainfont before -H header includes, which would ignore Scale.
                // Instead we load fontspec and call \setmainfont[Scale=X]{Font} ourselves.
                cmd += " --pdf-engine=xelatex";
            }
            ofstream hdr(headerFile);
            if (!hdr) {
                cerr << "Error: could not create header file '" << headerFile << "'." << endl;
                remove(tmpFile.c_str());
                return 1;
            }
            if (!pdfFont.empty()) {
                hdr << "\\usepackage{fontspec}\n";
                hdr << "\\setmainfont[Scale=" << scale << "]{" << pdfFont << "}\n";
            }
            if (!pdfColorRegistry.empty()) {
                hdr << "\\usepackage{xcolor}\n";
                for (const auto& entry : pdfColorRegistry) {
                    hdr << "\\definecolor{" << entry.second << "}{HTML}{" << entry.first << "}\n";
                }
            }
            hdr.close();
            cmd += " -H \"" + headerFile + "\"";
        }
        cmd += " \"" + tmpFile + "\" -o \"" + outputFile + "\"";
        int ret = system(cmd.c_str());
        remove(tmpFile.c_str());
        remove(headerFile.c_str());

        if (ret != 0) {
            cerr << "Error: PDF conversion failed." << endl;
            cerr << endl;
            cerr << "PDF generation requires pandoc and a LaTeX engine." << endl;
            cerr << "Install both with:" << endl;
            cerr << "  macOS:  brew install pandoc && brew install --cask basictex" << endl;
            cerr << "  Linux:  apt install pandoc texlive" << endl;
            cerr << endl;
            cerr << "After installing basictex, run: sudo tlmgr update --self" << endl;
            cerr << "Then open a new Terminal window — pdflatex is not found in existing sessions." << endl;
            cerr << "Or generate markdown and convert manually:" << endl;
            cerr << "  gospel --outputtype=md | pandoc -f markdown -o " << outputFile << endl;
            return 1;
        }
        cerr << "Saved to " << outputFile << endl;
        if (printPdf) {
            // Find the first available printer to avoid stale ~/.cups/lpoptions issues
            FILE* pipe = popen("lpstat -p 2>/dev/null | awk '/^printer/ {print $2; exit}'", "r");
            string printer;
            if (pipe) {
                char buf[256];
                if (fgets(buf, sizeof(buf), pipe)) {
                    printer = buf;
                    // trim trailing newline
                    if (!printer.empty() && printer.back() == '\n')
                        printer.pop_back();
                }
                pclose(pipe);
            }
            string printCmd;
            if (!printer.empty()) {
                printCmd = "lpr -P \"" + printer + "\" \"" + outputFile + "\"";
            } else {
                printCmd = "lpr \"" + outputFile + "\"";
            }
            int printRet = system(printCmd.c_str());
            if (printRet != 0) {
                cerr << "Warning: print command failed." << endl;
                cerr << "If you see a lpoptions error, fix it with: rm ~/.cups/lpoptions" << endl;
                cerr << "To list available printers: lpstat -p" << endl;
            } else {
                if (!printer.empty())
                    cerr << "Sent to printer: " << printer << endl;
                else
                    cerr << "Sent to printer." << endl;
            }
        }
    } else {
        ofstream f(outputFile);
        if (!f) {
            cerr << "Error: could not write to '" << outputFile << "'." << endl;
            return 1;
        }
        f << out.str();
        cerr << "Saved to " << outputFile << endl;
    }

    return 0;
}