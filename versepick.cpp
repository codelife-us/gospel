// versepick.cpp — Interactive Bible verse browser
// Build: g++ -std=c++11 -o versepick versepick.cpp
// Usage: versepick [-bv=KJV|BSB|WEB]
//
// Navigate with arrow keys. Enter/→ to select, ←/Backspace to go back, q to quit.
// Selecting a verse copies its reference (e.g. "John 3:16") to the clipboard.

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <cstdio>

#ifdef _WIN32
# include <conio.h>
# define HOME_ENV "USERPROFILE"
#else
# include <termios.h>
# include <unistd.h>
# include <sys/ioctl.h>
# define HOME_ENV "HOME"
#endif

using namespace std;

// ── Bible data ────────────────────────────────────────────────────────────────

static vector<string>          gBooks;
static map<string, vector<int>> gChapters;  // book      → sorted chapter numbers
static map<string, vector<int>> gVerses;    // "Book N"  → sorted verse numbers
static map<string, string>      gText;      // "Book N:V" → verse text

static void loadBible(const string& path) {
    ifstream f(path);
    string line;
    set<string> seen;
    map<string, set<int>> chapSet, verseSet;

    while (getline(f, line)) {
        if (line.size() >= 3 &&
            (unsigned char)line[0] == 0xEF &&
            (unsigned char)line[1] == 0xBB &&
            (unsigned char)line[2] == 0xBF)
            line = line.substr(3);
        if (!line.empty() && line.back() == '\r') line.pop_back();

        size_t tab = line.find('\t');
        if (tab == string::npos) continue;
        string ref  = line.substr(0, tab);
        string text = line.substr(tab + 1);

        size_t col = ref.rfind(':');
        if (col == string::npos) continue;
        string bc  = ref.substr(0, col);        // "Book N"
        int    v   = stoi(ref.substr(col + 1));
        size_t sp  = bc.rfind(' ');
        if (sp == string::npos) continue;
        string book = bc.substr(0, sp);
        int    ch   = stoi(bc.substr(sp + 1));

        if (!seen.count(book)) { seen.insert(book); gBooks.push_back(book); }
        chapSet[book].insert(ch);
        verseSet[bc].insert(v);
        gText[ref] = text;
    }

    for (auto it = chapSet.begin();  it != chapSet.end();  ++it)
        gChapters[it->first] = vector<int>(it->second.begin(), it->second.end());
    for (auto it = verseSet.begin(); it != verseSet.end(); ++it)
        gVerses[it->first]   = vector<int>(it->second.begin(), it->second.end());
}

// ── Terminal ──────────────────────────────────────────────────────────────────

#ifndef _WIN32
static struct termios s_orig;
static void restoreTerminal() { tcsetattr(STDIN_FILENO, TCSANOW, &s_orig); }
static void enableRawMode() {
    tcgetattr(STDIN_FILENO, &s_orig);
    atexit(restoreTerminal);
    struct termios r = s_orig;
    r.c_lflag &= ~(unsigned)(ICANON | ECHO);
    r.c_cc[VMIN] = 1; r.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &r);
}
static int termRows() {
    struct winsize w = {};
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_row > 6 ? (int)w.ws_row : 24;
}
static int termCols() {
    struct winsize w = {};
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col > 20 ? (int)w.ws_col : 80;
}
#else
static void enableRawMode() {}
static int  termRows() { return 24; }
static int  termCols() { return 80; }
#endif

enum Key { UP, DOWN, LEFT, RIGHT, ENTER, BACK, QUIT, OTHER };

static Key readKey() {
#ifdef _WIN32
    int c = _getch();
    if (c == 224 || c == 0) {
        int c2 = _getch();
        if (c2 == 72) return UP;
        if (c2 == 80) return DOWN;
        if (c2 == 75) return LEFT;
        if (c2 == 77) return RIGHT;
        return OTHER;
    }
    if (c == '\r')             return ENTER;
    if (c == 8)                return BACK;
    if (c == 'q' || c == 'Q') return QUIT;
    return OTHER;
#else
    char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return QUIT;
    if (c == '\033') {
        char s[2] = {};
        if (read(STDIN_FILENO, &s[0], 1) != 1) return OTHER;
        if (read(STDIN_FILENO, &s[1], 1) != 1) return OTHER;
        if (s[0] == '[') {
            if (s[1] == 'A') return UP;
            if (s[1] == 'B') return DOWN;
            if (s[1] == 'C') return RIGHT;
            if (s[1] == 'D') return LEFT;
        }
        return OTHER;
    }
    if (c == '\r' || c == '\n') return ENTER;
    if (c == 127 || c == '\b') return BACK;
    if (c == 'q' || c == 'Q') return QUIT;
    return OTHER;
#endif
}

// ── Clipboard ─────────────────────────────────────────────────────────────────

static void copyToClipboard(const string& text) {
#ifdef __APPLE__
    FILE* p = popen("pbcopy", "w");
    if (p) { fwrite(text.c_str(), 1, text.size(), p); pclose(p); }
#elif defined(_WIN32)
    string cmd = "echo " + text + "| clip";
    system(cmd.c_str());
#else
    FILE* p = popen("xclip -selection clipboard 2>/dev/null", "w");
    if (!p) p = popen("xsel --clipboard --input 2>/dev/null", "w");
    if (p) { fwrite(text.c_str(), 1, text.size(), p); pclose(p); }
#endif
}

// ── UI ────────────────────────────────────────────────────────────────────────

static void clrscr() { cout << "\033[2J\033[H" << flush; }

static void adjustScroll(int sel, int& scroll, int vis) {
    if (sel < scroll)          scroll = sel;
    if (sel >= scroll + vis)   scroll = sel - vis + 1;
    if (scroll < 0)            scroll = 0;
}

// Display a navigable list. sel is read/written so position persists.
// Returns: selected index, -1 (back/left), or -2 (quit).
static int browseList(const string& header, const vector<string>& items, int& sel) {
    if (items.empty()) return -1;
    if (sel >= (int)items.size()) sel = (int)items.size() - 1;

    int scroll = 0;
    while (true) {
        int vis = termRows() - 5;
        if (vis < 2) vis = 2;
        adjustScroll(sel, scroll, vis);

        clrscr();
        cout << "\033[1m" << header << "\033[0m\n\n";

        int end = min(scroll + vis, (int)items.size());
        for (int i = scroll; i < end; ++i) {
            if (i == sel)
                cout << " \033[7m " << items[i] << " \033[0m\n";
            else
                cout << "   " << items[i] << "\n";
        }

        cout << "\n\033[2m"
             << "\xe2\x86\x91\xe2\x86\x93"   // ↑↓
             << " navigate   Enter/" "\xe2\x86\x92" " select   "  // →
             << "\xe2\x86\x90" "/Backspace back   q quit"          // ←
             << "\033[0m\n";
        cout.flush();

        Key k = readKey();
        if      (k == UP    && sel > 0)                      --sel;
        else if (k == DOWN  && sel < (int)items.size() - 1)  ++sel;
        else if (k == ENTER || k == RIGHT)                    return sel;
        else if (k == BACK  || k == LEFT)                     return -1;
        else if (k == QUIT)                                   return -2;
    }
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    string version = "KJV";

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if      (arg.find("-bv=") == 0)            version = arg.substr(4);
        else if (arg.find("--bibleversion=") == 0) version = arg.substr(15);
        else if (arg == "-h" || arg == "--help") {
            cout << "versepick — interactive Bible verse browser\n\n"
                 << "Usage: versepick [-bv=KJV|BSB|WEB]\n\n"
                 << "Browse by book, chapter, and verse using arrow keys.\n"
                 << "Pressing Enter on a verse copies its reference to the clipboard.\n";
            return 0;
        }
    }

    transform(version.begin(), version.end(), version.begin(), ::toupper);

    string bibleFile, bibleUrl;
    if      (version == "KJV") { bibleFile = "BibleKJV.txt"; bibleUrl = "https://openbible.com/textfiles/kjv.txt"; }
    else if (version == "BSB") { bibleFile = "BibleBSB.txt"; bibleUrl = "https://bereanbible.com/bsb.txt"; }
    else if (version == "WEB") { bibleFile = "BibleWEB.txt"; bibleUrl = "https://openbible.com/textfiles/web.txt"; }
    else { cerr << "Unknown version '" << version << "'. Use KJV, BSB, or WEB.\n"; return 1; }

    // Resolve file: current dir, then HOME
    {
        ifstream test(bibleFile);
        if (!test.good()) {
            const char* home = getenv(HOME_ENV);
            if (home) {
                string hp = string(home) + "/" + bibleFile;
                ifstream ht(hp);
                if (ht.good()) bibleFile = hp;
            }
        }
    }
    {
        ifstream test(bibleFile);
        if (!test.good()) {
            cerr << "Bible file '" << bibleFile << "' not found.\n"
                 << "Download it now? (y/n): ";
            char ans; cin >> ans;
            if (ans == 'y' || ans == 'Y') {
                string cmd = "curl -L \"" + bibleUrl + "\" -o \"" + bibleFile + "\"";
                if (system(cmd.c_str()) != 0) { cerr << "Download failed.\n"; return 1; }
            } else {
                return 1;
            }
        }
    }

    cerr << "Loading " << bibleFile << "..." << flush;
    loadBible(bibleFile);
    cerr << " done.\n";

    if (gBooks.empty()) { cerr << "No verses loaded.\n"; return 1; }

    enableRawMode();

    int bookSel  = 0;
    int chapSel  = 0;
    int verseSel = 0;
    int state    = 0;   // 0 = book, 1 = chapter, 2 = verse

    while (true) {
        if (state == 0) {
            string header = "Bible Verse Picker  (" + version + ")  \xe2\x80\x94  Select Book";
            int r = browseList(header, gBooks, bookSel);
            if (r == -2) break;
            if (r >= 0)  { chapSel = 0; state = 1; }

        } else if (state == 1) {
            const string&     book = gBooks[bookSel];
            const vector<int>& chs = gChapters[book];
            vector<string> items;
            for (int c : chs) items.push_back("Chapter " + to_string(c));

            int r = browseList(book + "  \xe2\x80\xba  Select Chapter", items, chapSel);
            if (r == -2) break;
            if (r == -1) state = 0;
            else         { verseSel = 0; state = 2; }

        } else {
            const string&     book    = gBooks[bookSel];
            int               chapter = gChapters[book][chapSel];
            string            bc      = book + " " + to_string(chapter);
            const vector<int>& vs     = gVerses[bc];

            if (vs.empty()) { state = 1; continue; }

            // Build items: right-aligned verse number + beginning of verse text
            int numW = (int)to_string(vs.back()).size();
            int txtW = termCols() - numW - 6;   // 6: selection bar padding + two spaces
            if (txtW < 10) txtW = 10;

            vector<string> items;
            for (int v : vs) {
                string num = to_string(v);
                while ((int)num.size() < numW) num = " " + num;
                string ref  = bc + ":" + to_string(v);
                string text = gText.count(ref) ? gText[ref] : "";
                if ((int)text.size() > txtW) text = text.substr(0, txtW - 3) + "...";
                items.push_back(num + "  " + text);
            }

            string header = book + " " + to_string(chapter) + "  \xe2\x80\xba  Select Verse";
            int r = browseList(header, items, verseSel);
            if (r == -2) break;
            if (r == -1) { state = 1; }
            else {
                int    verse = vs[r];
                string ref   = bc + ":" + to_string(verse);
                copyToClipboard(ref);

                clrscr();
                cout << "\033[1mCopied to clipboard:\033[0m\n\n"
                     << "  " << ref << "\n\n"
                     << "\033[2m" << gText[ref] << "\033[0m\n\n"
                     << "Press any key to browse more, or q to quit.\n";
                cout.flush();

                if (readKey() == QUIT) break;
            }
        }
    }

    clrscr();
    return 0;
}
