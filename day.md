# day

A small C++ utility that prints the current day of the year (Jan 1 = 1) and opens a YouTube search for the day's video. Opening YouTube is the default behavior; use `-d` to print the day number only.

## Building

```bash
g++ -std=c++11 -o day day.cpp
```

## Usage

```bash
./day          # print day number and open YouTube search
./day -d       # print day number only, no YouTube
```

## Options

| Option | Description |
|--------|-------------|
| `-d`, `--day` | Print day number only, no YouTube |
| `-d=N`, `--day=N` | Use day N instead of today (still opens YouTube) |
| `-y`, `--youtube` | Open YouTube (explicit; already the default) |
| `-q=TEXT`, `--query=TEXT` | Override the YouTube search query (`{day}` = day number) |
| `-h`, `--help` | Show help |

## Config file (.day)

Create a `.day` file in the current directory or `$HOME` to set a default YouTube search query. The first non-blank, non-comment line is used. Lines starting with `#` are ignored.

```
# .day — default YouTube search query for day
Day {day} The Bible Recap
```

Query priority: `-q=` flag → `.day` in current dir → `.day` in `$HOME` → built-in default (`Day {day} The Bible Recap`).

## Examples

Open YouTube for today's Bible Recap:
```bash
./day
./day -y
```

Print day number only:
```bash
./day -d
./day --day
```

Open YouTube for a specific day:
```bash
./day -d=203
./day --day=203
```

Custom search query:
```bash
./day -q="Day {day} The Bible Recap"
./day --query="Day {day} The Bible Recap"
```

## Composing with other tools

Open today's Bible Recap on YouTube and print the reading plan:
```bash
./day && ./bv -d
```

Print today's reading plan (day number only, piped to bv):
```bash
./bv -d=$(./day -d)
```
