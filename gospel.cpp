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

using namespace std;

const string VERSION = "1.0";

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
// Available tracts
map<string, Tract> availableTracts = {
    {"The Romans Road", {"The Romans Road", romansRoadSections}}
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

string formatCitation(const string& verseText, const string& reference, const string& version, bool markdown, int refStyle) {
    string citation = reference + " (" + version + ")";
    string formatted;
    if (refStyle == 2) {
        formatted = verseText + " - " + citation;
    } else {
        formatted = verseText + "\n\u2014 " + citation;
    }
    if (markdown) {
        formatted = "*" + formatted + "*";
    }
    return formatted;
}

// Process markdown text with embedded bible references
string processMarkdownReferences(const string& text, const string& version, bool markdown, int refStyle, bool verseNumbers) {
    string result = text;
    regex refPattern("\\[([^\\]]+)\\]");
    smatch match;
    string::const_iterator searchStart(text.cbegin());
    size_t offset = 0;

    while (regex_search(searchStart, text.cend(), match, refPattern)) {
        string reference = match[1].str();

        string verseText = lookupVerses(reference, verseNumbers);
        if (!verseText.empty()) {
            string replacement = formatCitation(verseText, reference, version, markdown, refStyle);

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
    cout << "  --bibleversion=VERSION  Specify Bible version (KJV, BSB)" << endl;
    cout << "  --outputtype=TYPE       Set output type (plaintext, md)" << endl;
    cout << "  -tn=NAME                Set tract name (default: 'The Romans Road')" << endl;
    cout << "  --tractname=NAME        Specify tract presentation by name" << endl;
    cout << "  --ref=REF                Output a Bible reference directly (use comma to separate multiple)" << endl;
    cout << "                           REF formats: Book Ch:V  Book Ch:V-V  Book Ch:V-  Book Ch" << endl;
    cout << "  --refstyle=STYLE         Citation style: 1=new line (default), 2=inline" << endl;
    cout << "  --versenumbers, -vn      Prefix each verse with its verse number, e.g. [1]" << endl;
    cout << "\nExamples:" << endl;
    cout << "  gospel --outputtype=md                            Display tract output as markdown" << endl;
    cout << "  gospel                                            Display default tract in KJV" << endl;
    cout << "  gospel -bv=BSB                                    Display default tract in BSB" << endl;
    cout << "  gospel -tn=\"The Romans Road\" -bv=KJV              Display Romans Road in KJV" << endl;
    cout << "  gospel --ref=\"John 3:16\"                          Display a single verse" << endl;
    cout << "  gospel --ref=\"John 3:16,Romans 8:9-10\"           Display multiple verses" << endl;
    cout << "  gospel --ref=\"Romans 8:20-\"                       Display verse 20 to end of chapter" << endl;
    cout << "  gospel --ref=\"Romans 8\" -vn                       Display a full chapter with verse numbers" << endl;
}
int main(int argc, char* argv[]) {
    string version = "KJV";
    string tractName = "The Romans Road"; // Default tract
    string outputType = "plaintext";
    string refArg;
    int refStyle = 1;
    bool verseNumbers = false;

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
    } else {
        cerr << "Error: unsupported Bible version '" << version << "'." << endl;
        cerr << "Supported versions: KJV, BSB" << endl;
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

    bool markdown = false;
    if (outputType == "md" || outputType == "MD") {
        markdown = true;
    }

    // --ref mode: output only the requested Bible references
    if (!refArg.empty()) {
        // Split by comma, strip optional surrounding []
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
                cout << formatCitation(verseText, token, version, markdown, refStyle) << endl << endl;
            } else {
                cerr << "Reference not found: " << token << endl;
            }
        }
        return 0;
    }

    // Check if tract exists
    if (availableTracts.find(tractName) == availableTracts.end()) {
        cerr << "Error: Tract '" << tractName << "' not found." << endl;
        cerr << "Available tracts:" << endl;
        for (const auto& tract : availableTracts) {
            cerr << "  - " << tract.first << endl;
        }
        return 1;
    }

    // Get the selected tract
    const Tract& selectedTract = availableTracts[tractName];

    for (const auto& v : selectedTract.sections) {
        if (v.section_title.length() > 0) {
            if (markdown) {
                cout << "## " << v.section_title << endl;
            } else {
                cout << v.section_title << endl;
            }
        }
        if (v.text.length() > 0) {
            string processedText = processMarkdownReferences(v.text, version, markdown, refStyle, verseNumbers);
            cout << processedText << endl;
        }
    }
    return 0;
}