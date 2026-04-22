// fontlist — Interactive font browser
// Scans system font directories, renders a browser preview page,
// serves it via a local Python HTTP server, and prints the selected path.
//
// Build: g++ -std=c++17 -o fontlist fontlist.cpp
// Usage: ./fontlist [--sample="preview text"] [--port=7777]

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <filesystem>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>

namespace fs = std::filesystem;
using namespace std;

static string g_tempDir;
static pid_t  g_serverPid = -1;
static bool   g_cleaned   = false;

static void doCleanup() {
    if (g_cleaned) return;
    g_cleaned = true;
    if (g_serverPid > 0) {
        kill(g_serverPid, SIGTERM);
        waitpid(g_serverPid, nullptr, WNOHANG);
        g_serverPid = -1;
    }
    if (!g_tempDir.empty()) {
        fs::remove_all(g_tempDir);
        g_tempDir.clear();
    }
}

static void sigHandler(int) { doCleanup(); exit(0); }

struct FontInfo {
    string name, path, symlink, ext;
};

static string sanitize(const string& s) {
    string r;
    for (unsigned char c : s)
        r += (isalnum(c) || c == '-') ? (char)c : '_';
    return r;
}

static string htmlEsc(const string& s) {
    string r;
    for (char c : s) {
        switch (c) {
            case '&': r += "&amp;";  break;
            case '"': r += "&quot;"; break;
            case '<': r += "&lt;";   break;
            case '>': r += "&gt;";   break;
            default:  r += c;
        }
    }
    return r;
}

static vector<FontInfo> scanFonts() {
    vector<string> dirs = { "/System/Library/Fonts", "/Library/Fonts" };
    if (const char* h = getenv("HOME"))
        dirs.push_back(string(h) + "/Library/Fonts");

    vector<FontInfo> fonts;
    set<string> seen;

    for (const auto& dir : dirs) {
        if (!fs::exists(dir)) continue;
        try {
            for (const auto& e : fs::recursive_directory_iterator(
                     dir, fs::directory_options::skip_permission_denied)) {
                if (!e.is_regular_file()) continue;
                string ext = e.path().extension().string();
                for (char& c : ext) c = (char)tolower((unsigned char)c);
                if (ext != ".ttf" && ext != ".ttc" && ext != ".otf") continue;
                string path = e.path().string();
                if (!seen.insert(path).second) continue;
                FontInfo fi;
                fi.path    = path;
                fi.ext     = ext.substr(1);
                fi.name    = e.path().stem().string();
                fi.symlink = sanitize(fi.name) + ext;
                fonts.push_back(fi);
            }
        } catch (...) {}
    }

    sort(fonts.begin(), fonts.end(), [](const FontInfo& a, const FontInfo& b) {
        string an = a.name, bn = b.name;
        for (char& c : an) c = (char)tolower((unsigned char)c);
        for (char& c : bn) c = (char)tolower((unsigned char)c);
        return an < bn;
    });

    // Deduplicate symlink names
    map<string, int> cnt;
    for (auto& fi : fonts) {
        int n = cnt[fi.symlink]++;
        if (n > 0)
            fi.symlink = sanitize(fi.name) + "_" + to_string(n) + "." + fi.ext;
    }
    return fonts;
}

static string makePythonServer() {
    return R"python(#!/usr/bin/env python3
import http.server, urllib.parse, os, sys

PORT    = int(sys.argv[1]) if len(sys.argv) > 1 else 7777
BASE    = os.path.dirname(os.path.abspath(__file__))
SELFILE = os.path.join(BASE, 'selected.txt')

class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *a, **kw):
        super().__init__(*a, directory=BASE, **kw)
    def do_GET(self):
        if self.path.startswith('/pick'):
            q = urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query)
            if 'path' in q:
                with open(SELFILE, 'w') as f:
                    f.write(q['path'][0])
            self.send_response(200)
            self.end_headers()
            self.wfile.write(b'ok')
        else:
            super().do_GET()
    def log_message(self, *a):
        pass

with http.server.HTTPServer(('127.0.0.1', PORT), Handler) as httpd:
    httpd.serve_forever()
)python";
}

static string makeHtml(const vector<FontInfo>& fonts, const string& sampleText) {
    ostringstream ff, cards;

    for (size_t i = 0; i < fonts.size(); ++i) {
        const auto& fi = fonts[i];
        ff << "@font-face{font-family:'f" << i << "';src:url('fonts/"
           << fi.symlink << "');}\n";
        cards << "<div class=\"card\" data-name=\"" << htmlEsc(fi.name)
              << "\" data-path=\"" << htmlEsc(fi.path) << "\" onclick=\"pick(this)\">"
              << "<div class=\"preview\" style=\"font-family:'f" << i << "'\">"
              << htmlEsc(sampleText) << "</div>"
              << "<div class=\"fname\">" << htmlEsc(fi.name) << "</div>"
              << "<div class=\"fpath\">" << htmlEsc(fi.path) << "</div>"
              << "</div>\n";
    }

    ostringstream html;
    html << R"html(<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Font List</title>
<style>
:root{--bg:#161b22;--card:#21262d;--text:#c9d1d9;--dim:#8b949e;--accent:#58a6ff;--border:#30363d}
*{box-sizing:border-box;margin:0;padding:0}
body{background:var(--bg);color:var(--text);font-family:system-ui,-apple-system,sans-serif}
header{position:sticky;top:0;background:var(--bg);border-bottom:1px solid var(--border);
  padding:12px 16px;display:flex;gap:10px;align-items:center;z-index:10}
header input{background:var(--card);color:var(--text);border:1px solid var(--border);
  border-radius:6px;padding:7px 11px;font-size:14px;outline:none}
header input:focus{border-color:var(--accent)}
#fsearch{flex:1}
#fsample{flex:2}
#fcount{color:var(--dim);font-size:13px;white-space:nowrap}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(300px,1fr));
  gap:10px;padding:16px;padding-bottom:60px}
.card{background:var(--card);border:2px solid var(--border);border-radius:8px;
  padding:14px 16px;cursor:pointer;transition:border-color .12s,background .12s;
  user-select:none}
.card:hover{border-color:var(--accent)}
.card.sel{border-color:var(--accent);background:#1c2d3f}
.preview{font-size:26px;line-height:1.35;margin-bottom:6px;overflow:hidden;
  white-space:nowrap;text-overflow:ellipsis}
.fname{font-size:12px;color:#aaa;margin-bottom:3px}
.fpath{font-size:11px;color:var(--dim);font-family:monospace;overflow:hidden;
  text-overflow:ellipsis;white-space:nowrap}
.hidden{display:none!important}
#bar{position:fixed;bottom:0;left:0;right:0;height:44px;background:#0d1117;
  border-top:1px solid var(--border);display:flex;align-items:center;
  gap:10px;padding:0 16px;overflow:hidden}
#bar label{color:var(--dim);font-size:12px;white-space:nowrap}
#bar-path{font-family:monospace;font-size:13px;color:var(--accent);
  overflow:hidden;text-overflow:ellipsis;white-space:nowrap;flex:1}
#bar-copied{color:#3fb950;font-size:12px;opacity:0;transition:opacity .3s;
  white-space:nowrap}
</style>
<style id="ff">
)html"
    << ff.str()
    << R"html(</style>
</head>
<body>
<header>
  <input id="fsearch" type="text" placeholder="Search fonts…" oninput="doFilter()">
  <input id="fsample" type="text" placeholder="Sample text…"
    value=")html" << htmlEsc(sampleText) << R"html(" oninput="doSample()">
  <div id="fcount"></div>
</header>
<div class="grid" id="grid">
)html"
    << cards.str()
    << R"html(</div>
<div id="bar">
  <label>Selected:</label>
  <div id="bar-path">— click a font —</div>
  <div id="bar-copied">✓ copied</div>
</div>
<script>
function doFilter() {
  const q = document.getElementById('fsearch').value.toLowerCase();
  let n = 0;
  document.querySelectorAll('.card').forEach(c => {
    const hide = q && !c.dataset.name.toLowerCase().includes(q);
    c.classList.toggle('hidden', hide);
    if (!hide) n++;
  });
  document.getElementById('fcount').textContent = n + ' fonts';
}

function doSample() {
  const t = document.getElementById('fsample').value || 'Aa Bb Cc';
  document.querySelectorAll('.preview').forEach(el => el.textContent = t);
}

function pick(el) {
  document.querySelectorAll('.card.sel').forEach(c => c.classList.remove('sel'));
  el.classList.add('sel');
  const path = el.dataset.path;
  document.getElementById('bar-path').textContent = path;
  navigator.clipboard.writeText(path).then(() => {
    const c = document.getElementById('bar-copied');
    c.style.opacity = 1;
    setTimeout(() => c.style.opacity = 0, 1500);
  }).catch(() => {});
  fetch('/pick?path=' + encodeURIComponent(path)).catch(() => {});
}

doFilter();
</script>
</body>
</html>
)html";

    return html.str();
}

int main(int argc, char* argv[]) {
    string sampleText = "The quick brown fox";
    int    port       = 7777;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg.find("--sample=") == 0)
            sampleText = arg.substr(9);
        else if (arg.find("--port=") == 0)
            port = stoi(arg.substr(7));
        else if (arg == "-h" || arg == "--help") {
            cout << "Usage: fontlist [--sample=\"preview text\"] [--port=7777]\n"
                 << "  Scans system fonts, opens a browser preview page,\n"
                 << "  and prints the path of the last-clicked font on exit.\n";
            return 0;
        }
    }

    signal(SIGINT,  sigHandler);
    signal(SIGTERM, sigHandler);
    atexit(doCleanup);

    cerr << "Scanning fonts...\n";
    auto fonts = scanFonts();
    if (fonts.empty()) {
        cerr << "No fonts found.\n";
        return 1;
    }
    cerr << "Found " << fonts.size() << " fonts.\n";

    // Create temp dir
    char tmp[] = "/tmp/fontlist_XXXXXX";
    const char* td = mkdtemp(tmp);
    if (!td) { perror("mkdtemp"); return 1; }
    g_tempDir = td;

    // Symlink each font into temp/fonts/
    fs::create_directory(g_tempDir + "/fonts");
    for (const auto& fi : fonts)
        symlink(fi.path.c_str(), (g_tempDir + "/fonts/" + fi.symlink).c_str());

    // Write server.py and index.html
    { ofstream f(g_tempDir + "/server.py");   f << makePythonServer(); }
    { ofstream f(g_tempDir + "/index.html");  f << makeHtml(fonts, sampleText); }

    // Start Python server
    g_serverPid = fork();
    if (g_serverPid == 0) {
        execlp("python3", "python3",
               (g_tempDir + "/server.py").c_str(),
               to_string(port).c_str(), nullptr);
        _exit(1);
    }
    if (g_serverPid < 0) { perror("fork"); doCleanup(); return 1; }

    usleep(400000); // give the server time to bind

    string url = "http://localhost:" + to_string(port) + "/";
    system(("open \"" + url + "\"").c_str());
    cerr << "Opened " << url << "\n";
    cerr << "Click a font to select it. Press Enter here to quit.\n";

    cin.get();

    // Print the last selected path
    string selFile = g_tempDir + "/selected.txt";
    if (fs::exists(selFile)) {
        ifstream f(selFile);
        string path;
        getline(f, path);
        if (!path.empty())
            cout << path << "\n";
    }

    doCleanup();
    return 0;
}
