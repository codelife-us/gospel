// biblereader.cpp — Bible reader in the browser
// Build: g++ -std=c++11 -o biblereader biblereader.cpp
// Usage: ./biblereader [-bv=KJV|BSB|WEB] [--port=7778]
//
// Opens a local browser Bible reader. Click a verse to copy its reference
// to the clipboard. The last selected reference is printed to stdout on exit.

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <csignal>
#include <unistd.h>
#include <sys/wait.h>

#ifdef _WIN32
# define HOME_ENV "USERPROFILE"
#else
# define HOME_ENV "HOME"
#endif

using namespace std;

// ── Bible data ────────────────────────────────────────────────────────────────

static vector<string>           gBooks;
static map<string, vector<int>> gChapters;   // book     → sorted chapter list
static map<string, vector<int>> gVerses;     // "Book N" → sorted verse list
static map<string, string>      gText;       // "Book N:V" → verse text

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
        size_t col  = ref.rfind(':');
        if (col == string::npos) continue;
        string bc   = ref.substr(0, col);
        int    v    = stoi(ref.substr(col + 1));
        size_t sp   = bc.rfind(' ');
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

// ── Helpers ───────────────────────────────────────────────────────────────────

static string htmlEsc(const string& s) {
    string r;
    for (char c : s) {
        if      (c == '&') r += "&amp;";
        else if (c == '<') r += "&lt;";
        else if (c == '>') r += "&gt;";
        else if (c == '"') r += "&quot;";
        else               r += c;
    }
    return r;
}

static string jsEsc(const string& s) {
    string r;
    for (char c : s) {
        if      (c == '"')  r += "\\\"";
        else if (c == '\\') r += "\\\\";
        else if (c == '\n') r += "\\n";
        else if (c == '\r') r += "";
        else                r += c;
    }
    return r;
}

// ── Process management ────────────────────────────────────────────────────────

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
        system(("rm -rf \"" + g_tempDir + "\"").c_str());
        g_tempDir.clear();
    }
}

static void sigHandler(int) { doCleanup(); exit(0); }

// ── Python server ─────────────────────────────────────────────────────────────

static string makePythonServer() {
    return R"python(#!/usr/bin/env python3
import http.server, urllib.parse, os, sys

PORT    = int(sys.argv[1]) if len(sys.argv) > 1 else 7778
BASE    = os.path.dirname(os.path.abspath(__file__))
SELFILE = os.path.join(BASE, 'selected.txt')

class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *a, **kw):
        super().__init__(*a, directory=BASE, **kw)
    def do_GET(self):
        if self.path.startswith('/pick'):
            q = urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query)
            if 'ref' in q:
                with open(SELFILE, 'w') as f:
                    f.write(q['ref'][0])
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

// ── HTML / JS generation ──────────────────────────────────────────────────────

static string makeBibleJS() {
    ostringstream js;

    js << "const BOOKS=[";
    for (size_t i = 0; i < gBooks.size(); ++i) {
        if (i) js << ",";
        js << "\"" << jsEsc(gBooks[i]) << "\"";
    }
    js << "];\n";

    js << "const BIBLE={";
    for (size_t bi = 0; bi < gBooks.size(); ++bi) {
        const string& book = gBooks[bi];
        if (bi) js << ",";
        js << "\"" << jsEsc(book) << "\":{";
        const vector<int>& chs = gChapters[book];
        for (size_t ci = 0; ci < chs.size(); ++ci) {
            int ch = chs[ci];
            if (ci) js << ",";
            js << ch << ":{";
            string bc = book + " " + to_string(ch);
            const vector<int>& vs = gVerses[bc];
            for (size_t vi = 0; vi < vs.size(); ++vi) {
                int v = vs[vi];
                if (vi) js << ",";
                string ref = bc + ":" + to_string(v);
                js << v << ":\"" << jsEsc(gText[ref]) << "\"";
            }
            js << "}";
        }
        js << "}";
    }
    js << "};\n";

    return js.str();
}

static string makeHtml(const string& version) {
    ostringstream html;

    html << R"html(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>Bible Reader</title>
<style>
:root{--bg:#0d1117;--sb:#161b22;--text:#e6edf3;--dim:#8b949e;--accent:#58a6ff;
  --border:#30363d;--hover:#1c2d3f;--sel:#1a3a6b;--vn:#8b949e}
*{box-sizing:border-box;margin:0;padding:0}
body{background:var(--bg);color:var(--text);font-family:system-ui,sans-serif;
  display:flex;flex-direction:column;height:100vh;overflow:hidden}

/* ── header ── */
#hdr{display:flex;align-items:center;gap:12px;padding:10px 16px;
  border-bottom:1px solid var(--border);flex-shrink:0;background:var(--sb)}
#hdr-title{font-weight:600;font-size:15px;white-space:nowrap}
#hdr-ver{font-size:12px;color:var(--dim);white-space:nowrap}
#search{flex:1;background:var(--bg);color:var(--text);border:1px solid var(--border);
  border-radius:6px;padding:6px 10px;font-size:13px;outline:none}
#search:focus{border-color:var(--accent)}
#search-clear{background:none;border:none;color:var(--dim);cursor:pointer;
  font-size:16px;padding:0 4px;display:none}
#search-clear:hover{color:var(--text)}

/* ── layout ── */
#body{display:flex;flex:1;overflow:hidden}

/* ── sidebar ── */
#sidebar{width:190px;flex-shrink:0;overflow-y:auto;border-right:1px solid var(--border);
  background:var(--sb);padding:6px 0}
#sidebar::-webkit-scrollbar{width:6px}
#sidebar::-webkit-scrollbar-thumb{background:var(--border);border-radius:3px}
.book{border-bottom:1px solid #1c2128}
.bname{padding:5px 12px;font-size:13px;cursor:pointer;user-select:none;
  white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.bname:hover{background:var(--hover);color:var(--accent)}
.bname.open{color:var(--accent)}
.bchaps{display:none;padding:4px 8px 6px 12px;flex-wrap:wrap;gap:3px}
.bchaps.open{display:flex}
.chn{min-width:24px;padding:2px 4px;font-size:11px;text-align:center;cursor:pointer;
  border-radius:4px;color:var(--dim);user-select:none}
.chn:hover{background:var(--hover);color:var(--text)}
.chn.active{background:var(--accent);color:#0d1117;font-weight:600}

/* ── main ── */
#main{flex:1;overflow-y:auto;padding:0}
#main::-webkit-scrollbar{width:8px}
#main::-webkit-scrollbar-thumb{background:var(--border);border-radius:4px}
#ch-hdr{display:flex;align-items:center;gap:8px;padding:14px 28px 10px;
  border-bottom:1px solid var(--border);position:sticky;top:0;
  background:var(--bg);z-index:5}
#ch-title{font-size:18px;font-weight:600;flex:1}
.nav-btn{background:none;border:1px solid var(--border);color:var(--dim);
  border-radius:6px;padding:4px 10px;font-size:13px;cursor:pointer}
.nav-btn:hover{border-color:var(--accent);color:var(--accent)}
.nav-btn:disabled{opacity:0.25;cursor:default}
#text-block{padding:20px 28px 40px;line-height:1.85;font-size:16px;
  font-family:Georgia,'Times New Roman',serif}
.verse{cursor:pointer;border-radius:3px;padding:1px 2px;transition:background .1s}
.verse:hover{background:var(--hover)}
.verse.sel{background:var(--sel)}
sup.vn{font-size:10px;color:var(--vn);margin-right:2px;margin-left:3px;
  font-family:system-ui,sans-serif;user-select:none;vertical-align:super}

/* ── search results ── */
#results{padding:16px 28px;display:none}
.r-count{font-size:12px;color:var(--dim);margin-bottom:12px}
.r-item{padding:8px 0;border-bottom:1px solid var(--border);cursor:pointer}
.r-item:hover .r-ref{color:var(--accent)}
.r-ref{font-size:12px;color:var(--dim);font-family:monospace;margin-bottom:3px}
.r-text{font-size:14px;font-family:Georgia,serif;line-height:1.5}
.r-text em{background:#2d3e1f;color:#7ee787;font-style:normal;border-radius:2px;padding:0 2px}

/* ── status bar ── */
#bar{flex-shrink:0;height:38px;background:#010409;border-top:1px solid var(--border);
  display:flex;align-items:center;gap:10px;padding:0 16px;overflow:hidden}
#bar-ref{font-family:monospace;font-size:13px;color:var(--accent);
  overflow:hidden;text-overflow:ellipsis;white-space:nowrap;flex:1}
#bar-hint{font-size:11px;color:var(--dim);white-space:nowrap}
#bar-ok{color:#3fb950;font-size:12px;opacity:0;transition:opacity .3s;white-space:nowrap}
</style>
</head>
<body>

<div id="hdr">
  <div id="hdr-title">Bible Reader</div>
  <div id="hdr-ver">)html" << htmlEsc(version) << R"html(</div>
  <input id="search" type="text" placeholder="Search verses&#x2026;" oninput="onSearch()" spellcheck="false">
  <button id="search-clear" onclick="clearSearch()">&#x2715;</button>
</div>

<div id="body">
  <div id="sidebar"></div>
  <div id="main">
    <div id="ch-hdr" style="display:none">
      <button class="nav-btn" id="btn-prev" onclick="navChapter(-1)">&#x2190; Prev</button>
      <div id="ch-title"></div>
      <button class="nav-btn" id="btn-next" onclick="navChapter(1)">Next &#x2192;</button>
    </div>
    <div id="text-block"></div>
    <div id="results"></div>
  </div>
</div>

<div id="bar">
  <div id="bar-ref">Click a verse to select it</div>
  <div id="bar-hint">reference copied to clipboard</div>
  <div id="bar-ok">&#x2713; copied</div>
</div>

<script>
)html" << makeBibleJS() << R"html(

// ── state ──────────────────────────────────────────────────────────────────
let curBook = null, curCh = null;
let searchTimer = null;

function sanitizeId(s) { return s.replace(/[^a-zA-Z0-9]/g, '_'); }

function esc(s) {
    return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
}

// ── sidebar ────────────────────────────────────────────────────────────────
function buildSidebar() {
    const sb = document.getElementById('sidebar');
    BOOKS.forEach(book => {
        const chs = Object.keys(BIBLE[book]).map(Number).sort((a,b)=>a-b);

        const bookDiv  = document.createElement('div');
        bookDiv.className = 'book';

        const nameDiv  = document.createElement('div');
        nameDiv.className = 'bname';
        nameDiv.textContent = book;
        nameDiv.addEventListener('click', () => toggleBook(book));

        const chapsDiv = document.createElement('div');
        chapsDiv.className = 'bchaps';
        chapsDiv.id = 'bc-' + sanitizeId(book);

        chs.forEach(ch => {
            const span = document.createElement('span');
            span.className = 'chn';
            span.textContent = ch;
            span.id = 'chn-' + sanitizeId(book) + '-' + ch;
            span.addEventListener('click', e => { e.stopPropagation(); showChapter(book, ch); });
            chapsDiv.appendChild(span);
        });

        bookDiv.appendChild(nameDiv);
        bookDiv.appendChild(chapsDiv);
        sb.appendChild(bookDiv);
    });
}

function toggleBook(book) {
    const chapsDiv = document.getElementById('bc-' + sanitizeId(book));
    const nameDiv  = chapsDiv.previousElementSibling;
    const open = chapsDiv.classList.toggle('open');
    nameDiv.classList.toggle('open', open);
}

function ensureBookOpen(book) {
    const chapsDiv = document.getElementById('bc-' + sanitizeId(book));
    if (!chapsDiv.classList.contains('open')) toggleBook(book);
    // Scroll sidebar to book
    chapsDiv.parentElement.scrollIntoView({block:'nearest'});
}

// ── chapter view ───────────────────────────────────────────────────────────
function showChapter(book, ch, targetVerse) {
    clearSearch();
    curBook = book; curCh = ch;

    // Sidebar highlight
    document.querySelectorAll('.chn.active').forEach(e => e.classList.remove('active'));
    const chnEl = document.getElementById('chn-' + sanitizeId(book) + '-' + ch);
    if (chnEl) { chnEl.classList.add('active'); ensureBookOpen(book); }

    // Header
    document.getElementById('ch-hdr').style.display = 'flex';
    document.getElementById('ch-title').textContent  = book + ' ' + ch;
    updateNavButtons();

    // Verse text
    const chData = BIBLE[book][ch];
    const nums   = Object.keys(chData).map(Number).sort((a,b)=>a-b);
    const tb     = document.getElementById('text-block');
    tb.innerHTML = '';
    nums.forEach(v => {
        const ref  = book + ' ' + ch + ':' + v;
        const span = document.createElement('span');
        span.className = 'verse';
        span.dataset.ref = ref;
        span.innerHTML = '<sup class="vn">' + v + '</sup>' + esc(chData[v]) + ' ';
        span.addEventListener('click', () => pickVerse(span));
        tb.appendChild(span);
    });

    document.getElementById('results').style.display = 'none';
    tb.style.display = 'block';

    if (targetVerse) {
        const el = tb.querySelector('[data-ref="' + targetVerse + '"]');
        if (el) { el.classList.add('sel'); el.scrollIntoView({block:'center'}); }
        else     { tb.parentElement.scrollTop = 0; }
    } else {
        tb.parentElement.scrollTop = 0;
    }
}

function updateNavButtons() {
    const bi  = BOOKS.indexOf(curBook);
    const chs = Object.keys(BIBLE[curBook]).map(Number).sort((a,b)=>a-b);
    const ci  = chs.indexOf(curCh);
    document.getElementById('btn-prev').disabled = (bi === 0 && ci === 0);
    document.getElementById('btn-next').disabled = (bi === BOOKS.length-1 && ci === chs.length-1);
}

function navChapter(dir) {
    const chs = Object.keys(BIBLE[curBook]).map(Number).sort((a,b)=>a-b);
    let ci = chs.indexOf(curCh) + dir;
    let bi = BOOKS.indexOf(curBook);
    if (ci < 0) {
        bi--;
        if (bi < 0) return;
        const pchs = Object.keys(BIBLE[BOOKS[bi]]).map(Number).sort((a,b)=>a-b);
        showChapter(BOOKS[bi], pchs[pchs.length-1]);
    } else if (ci >= chs.length) {
        bi++;
        if (bi >= BOOKS.length) return;
        const nchs = Object.keys(BIBLE[BOOKS[bi]]).map(Number).sort((a,b)=>a-b);
        showChapter(BOOKS[bi], nchs[0]);
    } else {
        showChapter(curBook, chs[ci]);
    }
}

// ── verse pick ─────────────────────────────────────────────────────────────
function pickVerse(el) {
    document.querySelectorAll('.verse.sel').forEach(e => e.classList.remove('sel'));
    el.classList.add('sel');
    const ref = el.dataset.ref;
    document.getElementById('bar-ref').textContent = ref;
    navigator.clipboard.writeText(ref).then(() => {
        const ok = document.getElementById('bar-ok');
        ok.style.opacity = 1;
        setTimeout(() => ok.style.opacity = 0, 1500);
    }).catch(()=>{});
    fetch('/pick?ref=' + encodeURIComponent(ref)).catch(()=>{});
}

// ── search ─────────────────────────────────────────────────────────────────
function onSearch() {
    const q = document.getElementById('search').value.trim();
    document.getElementById('search-clear').style.display = q ? 'inline' : 'none';
    clearTimeout(searchTimer);
    if (!q) { restoreChapter(); return; }
    searchTimer = setTimeout(() => doSearch(q), 250);
}

function clearSearch() {
    document.getElementById('search').value = '';
    document.getElementById('search-clear').style.display = 'none';
    restoreChapter();
}

function restoreChapter() {
    document.getElementById('results').style.display = 'none';
    document.getElementById('text-block').style.display = curBook ? 'block' : 'none';
    document.getElementById('ch-hdr').style.display = curBook ? 'flex' : 'none';
}

function doSearch(q) {
    const ql = q.toLowerCase();
    const items = [];
    const MAX = 300;
    outer:
    for (const book of BOOKS) {
        const bdata = BIBLE[book];
        for (const ch in bdata) {
            const cdata = bdata[ch];
            for (const v in cdata) {
                const text = cdata[v];
                if (text.toLowerCase().includes(ql)) {
                    items.push({book, ch:parseInt(ch), v:parseInt(v), text,
                                ref: book+' '+ch+':'+v});
                    if (items.length >= MAX) break outer;
                }
            }
        }
    }

    const rd = document.getElementById('results');
    rd.innerHTML = '';
    rd.style.display = 'block';
    document.getElementById('text-block').style.display = 'none';
    document.getElementById('ch-hdr').style.display = 'none';

    const count = document.createElement('div');
    count.className = 'r-count';
    count.textContent = items.length === MAX
        ? 'Showing first ' + MAX + ' results for “' + q + '”'
        : items.length + ' result' + (items.length!==1?'s':'') + ' for “' + q + '”';
    rd.appendChild(count);

    if (!items.length) return;

    // Highlight matches in text
    const re = new RegExp('(' + q.replace(/[.*+?^${}()|[\]\\]/g,'\\$&') + ')', 'gi');
    items.forEach(({book, ch, v, text, ref}) => {
        const item = document.createElement('div');
        item.className = 'r-item';
        const refDiv  = document.createElement('div');
        refDiv.className = 'r-ref';
        refDiv.textContent = ref;
        const textDiv = document.createElement('div');
        textDiv.className = 'r-text';
        textDiv.innerHTML = esc(text).replace(re, '<em>$1</em>');
        item.appendChild(refDiv);
        item.appendChild(textDiv);
        item.addEventListener('click', () => showChapter(book, ch, ref));
        rd.appendChild(item);
    });
}

// ── keyboard navigation ────────────────────────────────────────────────────
document.addEventListener('keydown', e => {
    if (document.getElementById('search') === document.activeElement) return;
    if (e.key === 'ArrowLeft'  || (e.key === 'ArrowUp'   && e.altKey)) navChapter(-1);
    if (e.key === 'ArrowRight' || (e.key === 'ArrowDown'  && e.altKey)) navChapter(1);
});

// ── init ───────────────────────────────────────────────────────────────────
buildSidebar();
showChapter(BOOKS[0], 1);
</script>
</body>
</html>
)html";

    return html.str();
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    string version = "KJV";
    int    port    = 7778;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if      (arg.find("-bv=") == 0)            version = arg.substr(4);
        else if (arg.find("--bibleversion=") == 0) version = arg.substr(15);
        else if (arg.find("--port=") == 0)         port    = stoi(arg.substr(7));
        else if (arg == "-h" || arg == "--help") {
            cout << "Usage: biblereader [-bv=KJV|BSB|WEB] [--port=7778]\n"
                 << "  Opens a browser Bible reader.\n"
                 << "  Click a verse to copy its reference to the clipboard.\n"
                 << "  Press Enter here to quit; the last selected reference\n"
                 << "  is printed to stdout (for piping to bvi).\n";
            return 0;
        }
    }

    transform(version.begin(), version.end(), version.begin(), ::toupper);

    string bibleFile, bibleUrl;
    if      (version == "KJV") { bibleFile = "BibleKJV.txt"; bibleUrl = "https://openbible.com/textfiles/kjv.txt"; }
    else if (version == "BSB") { bibleFile = "BibleBSB.txt"; bibleUrl = "https://bereanbible.com/bsb.txt"; }
    else if (version == "WEB") { bibleFile = "BibleWEB.txt"; bibleUrl = "https://openbible.com/textfiles/web.txt"; }
    else { cerr << "Unknown version '" << version << "'. Use KJV, BSB, or WEB.\n"; return 1; }

    // Resolve file path
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
            cerr << "Bible file '" << bibleFile << "' not found.\nDownload it now? (y/n): ";
            char ans; cin >> ans;
            if (ans == 'y' || ans == 'Y') {
                string cmd = "curl -L \"" + bibleUrl + "\" -o \"" + bibleFile + "\"";
                if (system(cmd.c_str()) != 0) { cerr << "Download failed.\n"; return 1; }
            } else { return 1; }
        }
    }

    cerr << "Loading " << bibleFile << "..." << flush;
    loadBible(bibleFile);
    cerr << " " << gText.size() << " verses loaded.\n";

    if (gBooks.empty()) { cerr << "No verses loaded.\n"; return 1; }

    signal(SIGINT,  sigHandler);
    signal(SIGTERM, sigHandler);
    atexit(doCleanup);

    // Create temp dir
    char tmp[] = "/tmp/biblereader_XXXXXX";
    const char* td = mkdtemp(tmp);
    if (!td) { perror("mkdtemp"); return 1; }
    g_tempDir = td;

    cerr << "Generating page..." << flush;
    { ofstream f(g_tempDir + "/server.py");  f << makePythonServer(); }
    { ofstream f(g_tempDir + "/index.html"); f << makeHtml(version);  }
    cerr << " done.\n";

    // Fork Python server
    g_serverPid = fork();
    if (g_serverPid == 0) {
        execlp("python3", "python3",
               (g_tempDir + "/server.py").c_str(),
               to_string(port).c_str(), nullptr);
        _exit(1);
    }
    if (g_serverPid < 0) { perror("fork"); doCleanup(); return 1; }

    usleep(400000);

    string url = "http://localhost:" + to_string(port) + "/";
    system(("open \"" + url + "\"").c_str());
    cerr << "Opened " << url << "\n";
    cerr << "Press Enter to quit.\n";

    cin.get();

    // Print last selected reference
    string selFile = g_tempDir + "/selected.txt";
    ifstream sf(selFile);
    if (sf.good()) {
        string ref;
        getline(sf, ref);
        if (!ref.empty()) cout << ref << "\n";
    }

    doCleanup();
    return 0;
}
