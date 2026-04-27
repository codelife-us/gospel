// MIT License
// Copyright (c) 2026 Code Life
//
// day.cpp — Print the current day of the year (Jan 1 = 1)
// Build: g++ -std=c++11 -o day day.cpp
// Default: print day number and open YouTube Bible Recap search.
// -d/--day prints day number only (no YouTube).
// Query template loaded from .day in current dir or $HOME.
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <cstdlib>
#include <clocale>
using namespace std;

static int dayOfYear() {
    time_t t = time(nullptr);
    return localtime(&t)->tm_yday + 1;
}

static string urlEncode(const string& s) {
    string r;
    for (unsigned char c : s) {
        if (c == ' ')                                            r += '+';
        else if (isalnum(c) || c=='-'||c=='_'||c=='.'||c=='~') r += c;
        else { char buf[4]; snprintf(buf, sizeof(buf), "%%%02X", c); r += buf; }
    }
    return r;
}

static void openUrl(const string& url) {
#ifdef _WIN32
    system(("start \"\" \"" + url + "\"").c_str());
#elif defined(__APPLE__)
    system(("open \"" + url + "\"").c_str());
#else
    system(("xdg-open \"" + url + "\"").c_str());
#endif
}

// Read default query from .day file in current dir, then $HOME.
// Skips blank lines and lines starting with #.
static string loadQueryFile() {
    string paths[2] = {".day", ""};
    const char* home = getenv("HOME");
    if (home) paths[1] = string(home) + "/.day";

    for (const string& path : paths) {
        if (path.empty()) continue;
        ifstream f(path);
        if (!f.good()) continue;
        string line;
        while (getline(f, line)) {
            size_t s = line.find_first_not_of(" \t\r\n");
            if (s == string::npos || line[s] == '#') continue;
            size_t e = line.find_last_not_of(" \t\r\n");
            return line.substr(s, e - s + 1);
        }
    }
    return "";
}

int main(int argc, char* argv[]) {
    bool   dayOnly     = false;
    bool   planMode    = false;
    int    dayOverride = -1;   // -1 = use current day
    string queryTpl;           // empty = load from .day file or use built-in default

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            cout << "day — print the current day of the year (Jan 1 = 1)\n\n"
                 << "Usage: day [-d[=N]|--day[=N]] [-y|--youtube] [-q=TEXT|--query=TEXT] [-p|--plan]\n\n"
                 << "  (default)              Print day number and open YouTube The Bible Recap search\n"
                 << "  -d, --day              Print day number only, no YouTube\n"
                 << "  -d=N, --day=N          Use day N instead of today (still opens YouTube)\n"
                 << "  -y, --youtube          Open YouTube (explicit; already the default)\n"
                 << "  -q=TEXT, --query=TEXT  Override search query ({day} = day number)\n"
                 << "  -p, --plan             Print day number and run: bv --day --refonly\n"
                 << "  -h, --help             Show this help\n\n"
                 << "Config file (.day in current dir or $HOME):\n"
                 << "  First non-blank, non-comment line is used as the default query.\n"
                 << "  Lines starting with # are ignored.\n"
                 << "  Example contents:  Day {day} The Bible Recap\n\n"
                 << "Examples:\n"
                 << "  day                               # open YouTube for today's recap\n"
                 << "  day -d                            # print day number only\n"
                 << "  day -d=203                        # open YouTube for day 203\n"
                 << "  day -q=\"Day {day} The Bible Recap\" # custom query\n"
                 << "  day -p                            # print day number and run bv --day --refonly\n";
            return 0;
        } else if (arg.find("-d=") == 0 || arg.find("--day=") == 0) {
            dayOverride = stoi(arg.substr(arg.find('=') + 1));
            dayOnly = false;
        } else if (arg == "-d" || arg == "--day") {
            dayOnly = true;
        } else if (arg == "--youtube" || arg == "-y") {
            dayOnly = false;
        } else if (arg.find("--query=") == 0) {
            queryTpl = arg.substr(8);
        } else if (arg.find("-q=") == 0) {
            queryTpl = arg.substr(3);
        } else if (arg == "-p" || arg == "--plan") {
            planMode = true;
        }
    }

    int day = (dayOverride > 0) ? dayOverride : dayOfYear();
    cout << day << "\n";

    if (planMode) {
        setlocale(LC_TIME, "");
        time_t now = time(nullptr);
        struct tm target = *localtime(&now);
        target.tm_mon  = 0;
        target.tm_mday = day;  // mktime normalizes day-of-year into correct month/day
        target.tm_hour = 12;
        target.tm_min  = 0;
        target.tm_sec  = 0;
        mktime(&target);
        char dateBuf[64];
        strftime(dateBuf, sizeof(dateBuf), "%x", &target);
        cout << dateBuf << "\n";
        ifstream localBv("./bv");
        const string bvCmd = localBv.good() ? "./bv --day --refonly" : "bv --day --refonly";
        return system(bvCmd.c_str());
    }

    if (!dayOnly) {
        if (queryTpl.empty()) {
            queryTpl = loadQueryFile();
            if (queryTpl.empty()) queryTpl = "Day {day} The Bible Recap";
        }

        string q = queryTpl;
        string dayStr = to_string(day);
        size_t pos;
        while ((pos = q.find("{day}")) != string::npos)
            q.replace(pos, 5, dayStr);

        openUrl("https://www.youtube.com/results?search_query=" + urlEncode(q));
    }
}
